//===-- llvm-avm-image.cpp - AVM flat image packer -----------------------===//

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/BinaryFormat/AVM.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/DebugInfo/DWARF/DWARFDataExtractor.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugFrame.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugLine.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/CRC.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/FileOutputBuffer.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/WithColor.h"
#include <algorithm>
#include <optional>

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;

static cl::opt<std::string> Input(cl::Positional, cl::Required,
                                  cl::desc("<input.elf>"));
static cl::opt<std::string> Output("o", cl::value_desc("file"),
                                   cl::desc("Write image to <file>"),
                                   cl::Required);
static cl::alias OutputLong("output", cl::desc("Alias for -o"),
                            cl::aliasopt(Output));
static cl::opt<bool> Development(
    "development",
    cl::desc("Append a 4 KiB erased save area to a raw image"));
static cl::opt<std::string> Interpreter(
    "interpreter", cl::value_desc("interp.hex"),
    cl::desc("Interpreter HEX to include in .arduboy output"));
static cl::opt<std::string> Title(
    "title", cl::value_desc("text"),
    cl::desc("Title for .arduboy metadata (defaults to the ELF filename)"));
static cl::opt<std::string> Author(
    "author", cl::init("Unknown"), cl::value_desc("text"),
    cl::desc("Author for .arduboy metadata"));
static cl::opt<std::string> Description(
    "description", cl::value_desc("text"),
    cl::desc("Description for .arduboy metadata"));
static cl::opt<std::string> Genre(
    "genre", cl::value_desc("text"),
    cl::desc("Genre for .arduboy metadata"));
static cl::opt<std::string> Version(
    "game-version", cl::init("1.0"), cl::value_desc("text"),
    cl::desc("Version for .arduboy metadata"));
static cl::opt<bool> NoCrashInfo(
    "no-crash-info",
    cl::desc("Omit the compact AVD1 crash metadata and leave crashInfo zero"));

namespace {
constexpr uint32_t HeaderSize = 0x100;
constexpr uint32_t PageSize = 0x100;
constexpr uint32_t MaxDataSize = 1024;
constexpr uint32_t MaxProgramAddress = 0xffffff;
constexpr uint32_t MaxPayloadEnd = 0xfffef8;
constexpr uint32_t MaxFileSize = 0xffff00;
constexpr uint32_t MaxFlashSize = 0x1000000;
constexpr uint32_t DevelopmentSaveSize = 0x1000;
constexpr uint32_t AVMProgSpace = 0x10000000;
constexpr uint32_t AVMDataSpace = 0x20000000;
constexpr uint8_t RuntimeVersion = 4;
constexpr uint32_t CrashInfoOffset = 0x0f;
// AVD1 layout constants, shared with the guest reader in runtime/lib.
constexpr uint32_t CrashHeaderSize = 12;
constexpr uint32_t CrashSectionCount = 5;
constexpr uint32_t CrashDirectoryEntrySize = 7;
constexpr uint32_t CrashPayloadStart =
    CrashHeaderSize + CrashSectionCount * CrashDirectoryEntrySize;
constexpr uint32_t CrashFunctionSize = 9;
constexpr uint32_t CrashFileSize = 3;
constexpr uint32_t CrashCheckpointSize = 12;
constexpr uint32_t CrashCheckpointInterval = 64;
constexpr uint32_t CrashMaxRecords = 0xffff;
constexpr size_t CrashMaxFileBytes = 48;
// Opcodes 0x00-0x7f are one-byte combined transitions and need no constant.
constexpr uint8_t CrashOpSetMapping = 0x80;
constexpr uint8_t CrashOpLargeDelta = 0x81;
constexpr uint8_t CrashOpUnmapped = 0x82;
// 0x82 + PC delta (1-16) encodes a mapped same-file transition with a large
// line delta. 0x92 + PC delta (1-109) encodes an unmapped transition.
constexpr uint8_t CrashOpShortLargeBase = 0x82;
constexpr uint32_t CrashOpShortLargeMaxPC = 16;
constexpr uint8_t CrashOpShortUnmappedBase = 0x92;
constexpr uint32_t CrashOpShortUnmappedMaxPC = 109;

static void append16(SmallVectorImpl<uint8_t> &Out, uint16_t Value) {
  Out.push_back(Value);
  Out.push_back(Value >> 8);
}

static void append32(SmallVectorImpl<uint8_t> &Out, uint32_t Value) {
  append16(Out, Value);
  append16(Out, Value >> 16);
}

static std::string jsonString(StringRef Value) {
  std::string Result = "\"";
  constexpr char Hex[] = "0123456789abcdef";
  for (unsigned char C : Value) {
    if (C == '"' || C == '\\') {
      Result.push_back('\\');
      Result.push_back(C);
    } else if (C < 0x20) {
      Result += "\\u00";
      Result.push_back(Hex[C >> 4]);
      Result.push_back(Hex[C & 15]);
    } else {
      Result.push_back(C);
    }
  }
  return Result + '"';
}

struct ZipEntry {
  StringRef Name;
  ArrayRef<uint8_t> Data;
};

// .arduboy is a ZIP archive. Stored entries avoid a compression dependency
// and let tools extract the flash image byte for byte.
static SmallVector<uint8_t, 0> makeZip(ArrayRef<ZipEntry> Entries) {
  SmallVector<uint8_t, 0> Out;
  SmallVector<uint32_t, 4> Offsets;
  for (const ZipEntry &Entry : Entries) {
    Offsets.push_back(Out.size());
    append32(Out, 0x04034b50);
    append16(Out, 20); // Version needed.
    append16(Out, 0);  // Flags.
    append16(Out, 0);  // Stored.
    append16(Out, 0);  // DOS time.
    append16(Out, 0);  // DOS date.
    append32(Out, crc32(Entry.Data));
    append32(Out, Entry.Data.size());
    append32(Out, Entry.Data.size());
    append16(Out, Entry.Name.size());
    append16(Out, 0); // Extra length.
    llvm::append_range(Out, Entry.Name.bytes());
    llvm::append_range(Out, Entry.Data);
  }
  uint32_t CentralOffset = Out.size();
  for (size_t I = 0; I < Entries.size(); ++I) {
    const ZipEntry &Entry = Entries[I];
    append32(Out, 0x02014b50);
    append16(Out, 20); // Version made by.
    append16(Out, 20); // Version needed.
    append16(Out, 0);  // Flags.
    append16(Out, 0);  // Stored.
    append16(Out, 0);  // DOS time.
    append16(Out, 0);  // DOS date.
    append32(Out, crc32(Entry.Data));
    append32(Out, Entry.Data.size());
    append32(Out, Entry.Data.size());
    append16(Out, Entry.Name.size());
    append16(Out, 0); // Extra length.
    append16(Out, 0); // Comment length.
    append16(Out, 0); // Disk number.
    append16(Out, 0); // Internal attributes.
    append32(Out, 0); // External attributes.
    append32(Out, Offsets[I]);
    llvm::append_range(Out, Entry.Name.bytes());
  }
  uint32_t CentralSize = Out.size() - CentralOffset;
  append32(Out, 0x06054b50);
  append16(Out, 0); // Current disk.
  append16(Out, 0); // Central directory disk.
  append16(Out, Entries.size());
  append16(Out, Entries.size());
  append32(Out, CentralSize);
  append32(Out, CentralOffset);
  append16(Out, 0); // Comment length.
  return Out;
}

struct PayloadSection {
  StringRef Name;
  uint32_t Address;
  ArrayRef<uint8_t> Contents;
  uint64_t Flags;
};

static Error bad(const Twine &Message) {
  return createStringError(inconvertibleErrorCode(), Message);
}

static void write24(MutableArrayRef<uint8_t> Bytes, size_t Offset,
                    uint32_t Value) {
  Bytes[Offset] = Value;
  Bytes[Offset + 1] = Value >> 8;
  Bytes[Offset + 2] = Value >> 16;
}

static Expected<uint32_t> checkedEnd(uint64_t Address, uint64_t Size,
                                     StringRef Name) {
  if (Address > MaxProgramAddress || Size > MaxProgramAddress + 1ULL ||
      Address + Size > MaxProgramAddress + 1ULL)
    return bad(Twine("AVM section '") + Name +
               "' exceeds the 24-bit program-space limit");
  return static_cast<uint32_t>(Address + Size);
}

// Returns the final address of the single defined, externally visible
// __avm_trap_entry symbol, or 0 when the image has no trap handler.
static Expected<uint32_t>
findTrapEntry(const ELFFile<ELF32LE> &ELF,
              ArrayRef<ELF32LE::Shdr> Sections, StringRef InputName) {
  const char *TrapName = "__avm_trap_entry";
  uint32_t TrapEntry = 0;
  bool Found = false;
  for (const ELF32LE::Shdr &SymSec : Sections) {
    if (SymSec.sh_type != ELF::SHT_SYMTAB)
      continue;
    Expected<ArrayRef<ELF32LE::Sym>> Symbols =
        ELF.getSectionContentsAsArray<ELF32LE::Sym>(SymSec);
    if (!Symbols)
      return Symbols.takeError();
    Expected<StringRef> Strtab = ELF.getStringTableForSymtab(SymSec);
    if (!Strtab)
      return Strtab.takeError();
    for (const ELF32LE::Sym &Sym : *Symbols) {
      if (Sym.st_name >= Strtab->size())
        return bad(Twine("'") + InputName + "' has an invalid symbol name");
      StringRef Name = StringRef(*Strtab).drop_front(Sym.st_name)
                           .take_until([](char C) { return C == '\0'; });
      if (Name != TrapName || Sym.getBinding() == ELF::STB_LOCAL)
        continue;
      if (Found)
        return bad(Twine("multiple definitions of '") + TrapName + "'");
      Found = true;
      if (Sym.st_shndx == ELF::SHN_UNDEF || Sym.st_shndx == ELF::SHN_COMMON ||
          Sym.st_shndx == ELF::SHN_ABS || Sym.st_shndx >= Sections.size())
        return bad(Twine("'") + TrapName + "' must be a defined code symbol");
      const ELF32LE::Shdr &Sec = Sections[Sym.st_shndx];
      if (!(Sec.sh_flags & ELF::SHF_ALLOC) ||
          !(Sec.sh_flags & AVMProgSpace) ||
          !(Sec.sh_flags & ELF::SHF_EXECINSTR))
        return bad(Twine("'") + TrapName +
                   "' must lie in an executable AVM program-space section");
      if (Sym.st_value < 0x0100 || Sym.st_value > MaxProgramAddress)
        return bad(Twine("'") + TrapName +
                   "' must have a 24-bit address of at least 0x0100");
      if (Sym.st_value < Sec.sh_addr ||
          Sym.st_value - Sec.sh_addr >= Sec.sh_size)
        return bad(Twine("'") + TrapName +
                   "' lies outside its containing section");
      TrapEntry = static_cast<uint32_t>(Sym.st_value);
    }
  }
  return TrapEntry;
}

static Expected<uint32_t> finalFileSize(uint32_t PayloadEnd) {
  if (PayloadEnd > MaxPayloadEnd)
    return bad("AVM payload cannot be represented by a Version 1 flat image");
  uint64_t Size = alignTo(uint64_t(PayloadEnd) + 8, PageSize);
  if (Size > MaxFileSize || Size / PageSize > 0xffff)
    return bad("AVM image requires more than 0xFFFF pages");
  return static_cast<uint32_t>(Size);
}

static Error checkNoOverlap(ArrayRef<PayloadSection> Sections,
                            uint32_t StaticEnd) {
  SmallVector<PayloadSection, 8> Sorted(Sections.begin(), Sections.end());
  llvm::sort(Sorted, [](const PayloadSection &A, const PayloadSection &B) {
    return A.Address < B.Address;
  });
  uint32_t PreviousEnd = StaticEnd;
  StringRef PreviousName = ".saved/.data initializer";
  for (const PayloadSection &S : Sorted) {
    if (S.Address < PreviousEnd)
      return bad(Twine("AVM section '") + S.Name + "' overlaps '" +
                 PreviousName + "' in the flat image");
    Expected<uint32_t> End = checkedEnd(S.Address, S.Contents.size(), S.Name);
    if (!End)
      return End.takeError();
    PreviousEnd = *End;
    PreviousName = S.Name;
  }
  return Error::success();
}

static Expected<ArrayRef<uint8_t>> contents(const ELFFile<ELF32LE> &ELF,
                                             const ELF32LE::Shdr &Sec,
                                             StringRef Name) {
  Expected<ArrayRef<uint8_t>> Bytes = ELF.getSectionContents(Sec);
  if (!Bytes)
    return Bytes.takeError();
  if (Bytes->size() != Sec.sh_size)
    return bad(Twine("AVM section '") + Name + "' has unavailable contents");
  return *Bytes;
}

// One half-open program-space range in the AVD1 tables. Text is a function
// name for function ranges and a source basename for line ranges.
struct CodeRange {
  uint32_t Start;
  uint32_t End;
  std::string Text;
  uint32_t Line;
};

struct SymbolCandidate {
  uint32_t Start;
  uint32_t SectionEnd;
  uint32_t Size;
  bool Global;
  bool Synthetic;
  std::string Name;
};

static bool isCodeSection(const ELF32LE::Shdr &Sec) {
  return (Sec.sh_flags & ELF::SHF_ALLOC) && (Sec.sh_flags & AVMProgSpace) &&
         (Sec.sh_flags & ELF::SHF_EXECINSTR);
}

// The guest font draws printable ASCII only, so every other byte becomes '?'.
static std::string sanitizeCrashText(StringRef Text) {
  std::string Out;
  for (unsigned char C : Text)
    Out.push_back(C >= 0x20 && C < 0x7f ? C : '?');
  return Out;
}

// Demangles a function name in full. The guest font draws printable ASCII only,
// so every other byte becomes '?'. Names are never shortened or elided.
static std::string crashName(StringRef RawName) {
  return sanitizeCrashText(demangle(RawName));
}

// Shortens a basename to the limit while keeping its extension.
static std::string crashFileName(StringRef Path) {
  std::string Name = sanitizeCrashText(sys::path::filename(Path));
  if (Name.size() <= CrashMaxFileBytes)
    return Name;
  StringRef Ext = sys::path::extension(Name);
  if (Ext.size() >= CrashMaxFileBytes)
    return Name.substr(0, CrashMaxFileBytes);
  return Name.substr(0, CrashMaxFileBytes - Ext.size()) + Ext.str();
}

// Resolves overlaps in a range list sorted by start. A later start is
// authoritative, so an earlier range that reaches past it is truncated there.
// Empty results are dropped, which also removes exact duplicates.
static void clipOverlaps(SmallVectorImpl<CodeRange> &Ranges) {
  SmallVector<CodeRange, 0> Out;
  for (CodeRange &R : Ranges) {
    while (!Out.empty() && Out.back().End > R.Start) {
      Out.back().End = R.Start;
      if (Out.back().End <= Out.back().Start)
        Out.pop_back();
    }
    if (R.End > R.Start)
      Out.push_back(std::move(R));
  }
  Ranges = std::move(Out);
}

// Joins neighbours that share a source location. Input must be clipped.
static void coalesceLines(SmallVectorImpl<CodeRange> &Ranges) {
  SmallVector<CodeRange, 0> Out;
  for (CodeRange &R : Ranges) {
    if (!Out.empty() && Out.back().End == R.Start &&
        Out.back().Line == R.Line && Out.back().Text == R.Text) {
      Out.back().End = R.End;
      continue;
    }
    Out.push_back(std::move(R));
  }
  Ranges = std::move(Out);
}

// Function ranges come from final STT_FUNC symbols. A nonzero size is clamped
// to its section. An unsized function ends at the next function start in its
// section, or at the section end.
static Expected<SmallVector<CodeRange, 0>>
collectFunctions(const ELFFile<ELF32LE> &ELF,
                 ArrayRef<ELF32LE::Shdr> Sections, StringRef InputName) {
  SmallVector<SymbolCandidate, 0> Candidates;
  for (const ELF32LE::Shdr &SymSec : Sections) {
    if (SymSec.sh_type != ELF::SHT_SYMTAB)
      continue;
    Expected<ArrayRef<ELF32LE::Sym>> Symbols =
        ELF.getSectionContentsAsArray<ELF32LE::Sym>(SymSec);
    if (!Symbols)
      return Symbols.takeError();
    Expected<StringRef> Strtab = ELF.getStringTableForSymtab(SymSec);
    if (!Strtab)
      return Strtab.takeError();
    for (const ELF32LE::Sym &Sym : *Symbols) {
      if (Sym.getType() != ELF::STT_FUNC || Sym.st_shndx == ELF::SHN_UNDEF ||
          Sym.st_shndx >= Sections.size())
        continue;
      const ELF32LE::Shdr &Sec = Sections[Sym.st_shndx];
      uint64_t SectionEnd = uint64_t(Sec.sh_addr) + Sec.sh_size;
      if (!isCodeSection(Sec) || Sym.st_value < Sec.sh_addr ||
          Sym.st_value >= SectionEnd)
        continue;
      if (Sym.st_name >= Strtab->size())
        return bad(Twine("'") + InputName + "' has an invalid symbol name");
      StringRef Name = StringRef(*Strtab).drop_front(Sym.st_name)
                           .take_until([](char C) { return C == '\0'; });
      if (Name.empty())
        continue;
      std::string Demangled = crashName(Name);
      if (Demangled.empty())
        continue;
      Candidates.push_back({Sym.st_value, static_cast<uint32_t>(SectionEnd),
                            Sym.st_size, Sym.getBinding() != ELF::STB_LOCAL,
                            Name.starts_with(".L") || Name.starts_with("$"),
                            Demangled});
    }
  }

  // Aliases at one start compete. Sized, externally visible, nonsynthetic
  // names win, and the name breaks any remaining tie.
  llvm::sort(Candidates, [](const SymbolCandidate &A, const SymbolCandidate &B) {
    if (A.Start != B.Start)
      return A.Start < B.Start;
    if ((A.Size != 0) != (B.Size != 0))
      return A.Size != 0;
    if (A.Global != B.Global)
      return A.Global;
    if (A.Synthetic != B.Synthetic)
      return !A.Synthetic;
    return A.Name < B.Name;
  });

  SmallVector<CodeRange, 0> Ranges;
  SmallVector<SymbolCandidate, 0> Canonical;
  for (SymbolCandidate &C : Candidates)
    if (Canonical.empty() || Canonical.back().Start != C.Start)
      Canonical.push_back(std::move(C));

  for (size_t I = 0; I < Canonical.size(); ++I) {
    const SymbolCandidate &C = Canonical[I];
    uint64_t End = C.SectionEnd;
    if (C.Size != 0)
      End = std::min<uint64_t>(uint64_t(C.Start) + C.Size, C.SectionEnd);
    else if (I + 1 < Canonical.size() && Canonical[I + 1].Start < C.SectionEnd)
      End = Canonical[I + 1].Start;
    if (End <= C.Start)
      continue;
    if (End > MaxProgramAddress)
      return bad(Twine("function '") + C.Name +
                 "' ends beyond the 24-bit program address space");
    Ranges.push_back({C.Start, static_cast<uint32_t>(End), C.Name, 0});
  }
  clipOverlaps(Ranges);
  return Ranges;
}

// Line ranges come from the final DWARF line tables. Each ordinary row runs to
// the next row's address, and an end_sequence row closes its sequence without
// creating a location. Only executable program space keeps its ranges.
static Expected<SmallVector<CodeRange, 0>>
collectLines(const ObjectFile &Obj, ArrayRef<ELF32LE::Shdr> Sections) {
  std::unique_ptr<DWARFContext> Ctx = DWARFContext::create(Obj);
  SmallVector<CodeRange, 0> Ranges;
  auto Warn = [](Error E) {
    WithColor::warning(errs(), "llvm-avm-image") << toString(std::move(E))
                                                  << '\n';
  };
  for (const std::unique_ptr<DWARFUnit> &Unit : Ctx->compile_units()) {
    Expected<const DWARFDebugLine::LineTable *> Table =
        Ctx->getLineTableForUnit(Unit.get(), Warn);
    if (!Table)
      return Table.takeError();
    if (!*Table)
      continue;
    const DWARFDebugLine::LineTable &LT = **Table;
    for (const DWARFDebugLine::Sequence &Seq : LT.Sequences) {
      if (Seq.Empty || Seq.FirstRowIndex >= Seq.LastRowIndex ||
          Seq.LastRowIndex > LT.Rows.size())
        continue;
      for (unsigned I = Seq.FirstRowIndex; I + 1 < Seq.LastRowIndex; ++I) {
        const DWARFDebugLine::Row &Row = LT.Rows[I];
        if (Row.EndSequence || Row.Line == 0)
          continue;
        uint64_t Start = Row.Address.Address;
        uint64_t End = LT.Rows[I + 1].Address.Address;
        if (End <= Start)
          continue;
        std::string Path;
        if (!LT.getFileNameByIndex(
                Row.File, "",
                DILineInfoSpecifier::FileLineInfoKind::BaseNameOnly, Path))
          continue;
        if (Row.Line > 0xffffff)
          return bad("source line exceeds the 24-bit AVD1 line field");
        std::string File = crashFileName(Path);
        if (File.empty())
          continue;
        for (const ELF32LE::Shdr &Sec : Sections) {
          if (!isCodeSection(Sec))
            continue;
          uint64_t SectionEnd = uint64_t(Sec.sh_addr) + Sec.sh_size;
          uint64_t S = std::max<uint64_t>(Start, Sec.sh_addr);
          uint64_t E = std::min<uint64_t>(End, SectionEnd);
          if (S >= E)
            continue;
          if (E > MaxProgramAddress)
            return bad("source line range ends beyond the 24-bit program "
                       "address space");
          Ranges.push_back({static_cast<uint32_t>(S), static_cast<uint32_t>(E),
                            File, Row.Line});
        }
      }
    }
  }
  llvm::sort(Ranges, [](const CodeRange &A, const CodeRange &B) {
    return std::tie(A.Start, A.End, A.Text, A.Line) <
           std::tie(B.Start, B.End, B.Text, B.Line);
  });
  clipOverlaps(Ranges);
  coalesceLines(Ranges);
  return Ranges;
}

static void append24Value(SmallVectorImpl<uint8_t> &Out, uint64_t Value) {
  Out.push_back(Value);
  Out.push_back(Value >> 8);
  Out.push_back(Value >> 16);
}

// Appends the shortest unsigned LEB128 encoding of Value.
static void appendULEB128(SmallVectorImpl<uint8_t> &Out, uint32_t Value) {
  do {
    uint8_t Byte = Value & 0x7f;
    Value >>= 7;
    if (Value)
      Byte |= 0x80;
    Out.push_back(Byte);
  } while (Value);
}

// Deduplicates strings. Each record is a canonical ULEB128 byte length, the
// characters, and a NUL terminator. A reference names the length prefix, so the
// guest can validate a record from its bounded prefix without scanning it.
// Offsets are relative to the pool start.
class CrashStringPool {
public:
  uint32_t intern(StringRef Text) {
    auto [It, Inserted] = Offsets.try_emplace(Text, Bytes.size());
    if (Inserted) {
      appendULEB128(Bytes, Text.size());
      llvm::append_range(Bytes, Text.bytes());
      Bytes.push_back(0);
    }
    return It->getValue();
  }
  ArrayRef<uint8_t> bytes() const { return Bytes; }

private:
  SmallVector<uint8_t, 0> Bytes;
  StringMap<uint32_t> Offsets;
};

// A source state change. The state applies from PC up to the next transition.
// An unmapped transition retains the File and Line of the interval before it,
// so the next mapping in the same file can resume without an absolute line.
struct SourceTransition {
  uint32_t PC;
  bool Mapped;
  uint16_t File;
  uint32_t Line;
};

// The state decoding restarts from at a sparse checkpoint.
struct SourceCheckpoint {
  uint32_t PC;
  uint32_t StreamOffset;
  uint16_t File;
  uint32_t Line;
  bool Mapped;
};

static bool sameSourceState(const SourceTransition &A,
                            const SourceTransition &B) {
  if (A.Mapped != B.Mapped)
    return false;
  return !A.Mapped || (A.File == B.File && A.Line == B.Line);
}

// Keeps the transition list canonical: one state per PC, and no transition that
// repeats the state before it. The initial state is unmapped at PC zero.
static void appendSourceTransition(SmallVectorImpl<SourceTransition> &Out,
                                   SourceTransition T) {
  if (!Out.empty() && Out.back().PC == T.PC)
    Out.pop_back();
  if (Out.empty() ? !T.Mapped : sameSourceState(Out.back(), T))
    return;
  Out.push_back(T);
}

// Converts canonical source ranges into transitions. Adjacent ranges run on
// without an unmapped transition, and each gap is marked at its start. File
// IDs follow first occurrence in the range order.
static Error buildSourceTransitions(ArrayRef<CodeRange> Lines,
                                    SmallVectorImpl<StringRef> &Files,
                                    SmallVectorImpl<SourceTransition> &Out) {
  StringMap<uint16_t> FileIds;
  for (size_t I = 0; I < Lines.size(); ++I) {
    const CodeRange &R = Lines[I];
    auto [It, Inserted] = FileIds.try_emplace(R.Text, Files.size());
    if (Inserted) {
      if (Files.size() >= CrashMaxRecords)
        return bad("crash metadata exceeds 65535 source filenames");
      Files.push_back(R.Text);
    }
    appendSourceTransition(Out, {R.Start, true, It->getValue(), R.Line});
    bool GapFollows = I + 1 == Lines.size() || Lines[I + 1].Start != R.End;
    if (GapFollows && R.End <= MaxProgramAddress)
      appendSourceTransition(Out, {R.End, false, It->getValue(), R.Line});
  }
  return Error::success();
}

// Maps a signed value to an unsigned one, so small magnitudes stay small.
static uint32_t zigzagEncode(int32_t Value) {
  return (uint32_t(Value) << 1) ^ (Value < 0 ? ~0u : 0u);
}

// Encodes transitions as opcodes. Offsets receives the start of each opcode,
// relative to the first opcode. File and Line track the last mapped state, which
// an unmapped transition keeps, so a same-file mapping can resume from it.
static Error encodeSourceStream(ArrayRef<SourceTransition> Transitions,
                                SmallVectorImpl<uint8_t> &Stream,
                                SmallVectorImpl<uint32_t> &Offsets) {
  uint32_t PC = 0;
  uint32_t Line = 0;
  uint16_t File = 0;
  for (const SourceTransition &T : Transitions) {
    if (T.PC <= PC)
      return bad("source transitions do not strictly increase");
    uint32_t PCDelta = T.PC - PC;
    Offsets.push_back(Stream.size());
    if (!T.Mapped) {
      if (PCDelta <= CrashOpShortUnmappedMaxPC) {
        Stream.push_back(CrashOpShortUnmappedBase + PCDelta);
      } else {
        Stream.push_back(CrashOpUnmapped);
        appendULEB128(Stream, PCDelta);
      }
    } else if (Line == 0 || T.File != File) {
      Stream.push_back(CrashOpSetMapping);
      appendULEB128(Stream, PCDelta);
      appendULEB128(Stream, T.File);
      appendULEB128(Stream, T.Line);
    } else {
      int32_t LineDelta = int32_t(T.Line) - int32_t(Line);
      if (PCDelta <= 16 && LineDelta >= -4 && LineDelta <= 3) {
        Stream.push_back(uint8_t((uint32_t(LineDelta + 4) << 4) | (PCDelta - 1)));
      } else if (PCDelta <= CrashOpShortLargeMaxPC) {
        Stream.push_back(CrashOpShortLargeBase + PCDelta);
        appendULEB128(Stream, zigzagEncode(LineDelta));
      } else {
        Stream.push_back(CrashOpLargeDelta);
        appendULEB128(Stream, PCDelta);
        appendULEB128(Stream, zigzagEncode(LineDelta));
      }
    }
    PC = T.PC;
    if (T.Mapped) {
      File = T.File;
      Line = T.Line;
    }
  }
  return Error::success();
}

// Captures the state after every 64th transition, plus the initial state. Each
// checkpoint records the offset of the opcode that follows it.
static SmallVector<SourceCheckpoint, 0>
buildSourceCheckpoints(ArrayRef<SourceTransition> Transitions,
                       ArrayRef<uint32_t> Offsets, uint32_t StreamBytes) {
  SmallVector<SourceCheckpoint, 0> Checkpoints;
  Checkpoints.push_back({0, 0, 0, 0, false});
  for (size_t Count = CrashCheckpointInterval; Count <= Transitions.size();
       Count += CrashCheckpointInterval) {
    const SourceTransition &T = Transitions[Count - 1];
    uint32_t Next = Count < Transitions.size() ? Offsets[Count] : StreamBytes;
    Checkpoints.push_back({T.PC, Next, T.File, T.Line, T.Mapped});
  }
  return Checkpoints;
}

// Encodes the AVD1 block: a 12-byte header, a five-entry section directory, and
// the sections in directory order. Each section size is exact, with no padding.
static Expected<SmallVector<uint8_t, 0>>
encodeCrashInfo(ArrayRef<CodeRange> Functions, ArrayRef<CodeRange> Lines) {
  if (Functions.size() > CrashMaxRecords)
    return bad("crash metadata exceeds 65535 function records");

  // Function names are interned first, then filenames in first-occurrence order.
  CrashStringPool Strings;
  SmallVector<uint32_t, 0> NameOffsets;
  for (const CodeRange &F : Functions)
    NameOffsets.push_back(Strings.intern(F.Text));

  SmallVector<StringRef, 0> Files;
  SmallVector<SourceTransition, 0> Transitions;
  if (Error E = buildSourceTransitions(Lines, Files, Transitions))
    return std::move(E);
  SmallVector<uint32_t, 0> FileOffsets;
  for (StringRef File : Files)
    FileOffsets.push_back(Strings.intern(File));

  SmallVector<uint8_t, 0> Stream;
  SmallVector<uint32_t, 0> Offsets;
  if (Error E = encodeSourceStream(Transitions, Stream, Offsets))
    return std::move(E);
  if (Transitions.size() > 0xffffff)
    return bad("crash metadata exceeds 2^24 source transitions");
  SmallVector<SourceCheckpoint, 0> Checkpoints =
      buildSourceCheckpoints(Transitions, Offsets, Stream.size());
  if (Checkpoints.size() > CrashMaxRecords)
    return bad("crash metadata exceeds 65535 checkpoints");

  uint64_t SectionSizes[CrashSectionCount] = {
      2 + Functions.size() * CrashFunctionSize,
      2 + Files.size() * CrashFileSize,
      2 + Checkpoints.size() * CrashCheckpointSize,
      3 + Stream.size(),
      Strings.bytes().size(),
  };
  uint64_t Size = CrashPayloadStart;
  for (uint64_t SectionSize : SectionSizes)
    Size += SectionSize;
  if (Size > 0xffffff)
    return bad("crash metadata exceeds the 24-bit AVD1 size field");

  SmallVector<uint8_t, 0> Out;
  Out.append({'A', 'V', 'D', '1'});
  append24Value(Out, Size);
  Out.push_back(CrashSectionCount); // Section count.
  Out.push_back(0);                 // Flags.
  Out.append({0, 0, 0});            // Reserved.

  uint64_t SectionOffset = CrashPayloadStart;
  for (uint8_t Type = 1; Type <= CrashSectionCount; ++Type) {
    Out.push_back(Type);
    append24Value(Out, SectionOffset);
    append24Value(Out, SectionSizes[Type - 1]);
    SectionOffset += SectionSizes[Type - 1];
  }

  // Type 1: function index.
  append16(Out, Functions.size());
  for (size_t I = 0; I < Functions.size(); ++I) {
    append24Value(Out, Functions[I].Start);
    append24Value(Out, Functions[I].End);
    append24Value(Out, NameOffsets[I]);
  }

  // Type 2: filename index.
  append16(Out, Files.size());
  for (uint32_t Offset : FileOffsets)
    append24Value(Out, Offset);

  // Type 3: source checkpoints.
  append16(Out, Checkpoints.size());
  for (const SourceCheckpoint &C : Checkpoints) {
    append24Value(Out, C.PC);
    append24Value(Out, C.StreamOffset);
    append16(Out, C.File);
    append24Value(Out, C.Line);
    Out.push_back(C.Mapped);
  }

  // Type 4: source transition stream.
  append24Value(Out, Transitions.size());
  llvm::append_range(Out, Stream);

  // Type 5: string pool.
  llvm::append_range(Out, Strings.bytes());
  return Out;
}

// Crash metadata is best effort. A failure is reported and the image is packed
// without that part, so the image still packs as it did before the metadata
// existed. Function names do not depend on DWARF, and source lines are kept
// only when the DWARF line tables are usable.
static void warnCrashInfo(StringRef InputName, StringRef What, Error E) {
  WithColor::warning(errs(), "llvm-avm-image")
      << InputName << ": " << What << " omitted: " << toString(std::move(E))
      << '\n';
}

// Returns an empty block when no usable function or source-line data remains,
// or when the block cannot fit in Room bytes. If the complete block does not
// fit, the source lines are dropped and the function records are kept if they
// fit alone.
static SmallVector<uint8_t, 0> buildCrashInfo(const ObjectFile &Obj,
                                              const ELFFile<ELF32LE> &ELF,
                                              ArrayRef<ELF32LE::Shdr> Sections,
                                              StringRef InputName,
                                              uint64_t Room) {
  SmallVector<CodeRange, 0> Functions, Lines;
  if (Expected<SmallVector<CodeRange, 0>> Found =
          collectFunctions(ELF, Sections, InputName))
    Functions = std::move(*Found);
  else
    warnCrashInfo(InputName, "function names", Found.takeError());
  if (Expected<SmallVector<CodeRange, 0>> Found = collectLines(Obj, Sections))
    Lines = std::move(*Found);
  else
    warnCrashInfo(InputName, "source lines", Found.takeError());
  if (Functions.empty() && Lines.empty())
    return {};

  Expected<SmallVector<uint8_t, 0>> Block = encodeCrashInfo(Functions, Lines);
  if (Block && Block->size() <= Room)
    return std::move(*Block);
  Error Reason = Block ? bad("it does not fit in the Version 1 flat image")
                       : Block.takeError();
  if (Lines.empty() || Functions.empty()) {
    warnCrashInfo(InputName, "crash metadata", std::move(Reason));
    return {};
  }

  warnCrashInfo(InputName, "source lines", std::move(Reason));
  Block = encodeCrashInfo(Functions, {});
  if (Block && Block->size() <= Room)
    return std::move(*Block);
  warnCrashInfo(InputName, "crash metadata",
                Block ? bad("it does not fit in the Version 1 flat image")
                      : Block.takeError());
  return {};
}

// The guest unwinder reads AVEH3 through this program-space anchor. The runtime
// defines it as three zero bytes, and the packer patches them with the block
// address. A program without the exception runtime has no anchor and no block.
static constexpr const char *ExceptionAnchorName = "__avm_eh_metadata_ptr";

// AVEH3 layout constants, shared with runtime/lib/eh_unwind.c.
constexpr uint32_t EHFunctionHeaderSize = 18;
constexpr uint32_t EHRowHeaderSize = 5;
// A row has at most four saved registers, two bytes each.
constexpr uint32_t EHRowMaxSize = 13;
constexpr uint32_t EHCallRecordSize = 5;
constexpr uint32_t EHSiteSize = 10;
constexpr uint8_t EHFlagR3Base = 0x80;
constexpr uint8_t EHActionNone = 0;
constexpr uint8_t EHActionCatchAll = 1;
constexpr uint8_t EHActionCleanup = 2;
// Intermediate .avm.eh_sites records: u24 start, u24 end, u16 count, sites.
constexpr uint32_t EHSitesRecordHeaderSize = 8;
// Intermediate .avm.eh_calls records: u24 start, u16 count, then call entries.
constexpr uint32_t EHCallsRecordHeaderSize = 5;
// DWARF register numbers for the AVM registers used by the unwinder.
constexpr uint32_t DwarfR3 = 3;
constexpr uint32_t DwarfSP = 8;

// Returns the address of the defined anchor, or no value when the program does
// not reference the exception runtime.
static Expected<std::optional<uint32_t>>
findExceptionAnchor(const ELFFile<ELF32LE> &ELF,
                    ArrayRef<ELF32LE::Shdr> Sections, StringRef InputName) {
  std::optional<uint32_t> Anchor;
  for (const ELF32LE::Shdr &SymSec : Sections) {
    if (SymSec.sh_type != ELF::SHT_SYMTAB)
      continue;
    Expected<ArrayRef<ELF32LE::Sym>> Symbols =
        ELF.getSectionContentsAsArray<ELF32LE::Sym>(SymSec);
    if (!Symbols)
      return Symbols.takeError();
    Expected<StringRef> Strtab = ELF.getStringTableForSymtab(SymSec);
    if (!Strtab)
      return Strtab.takeError();
    for (const ELF32LE::Sym &Sym : *Symbols) {
      if (Sym.st_name >= Strtab->size())
        return bad(Twine("'") + InputName + "' has an invalid symbol name");
      StringRef Name = StringRef(*Strtab).drop_front(Sym.st_name)
                           .take_until([](char C) { return C == '\0'; });
      if (Name != ExceptionAnchorName)
        continue;
      if (Anchor)
        return bad(Twine("multiple definitions of '") +
                   ExceptionAnchorName + "'");
      if (Sym.st_shndx == ELF::SHN_UNDEF || Sym.st_shndx == ELF::SHN_COMMON ||
          Sym.st_shndx == ELF::SHN_ABS || Sym.st_shndx >= Sections.size())
        return bad(Twine("'") + ExceptionAnchorName +
                   "' must be a defined program-space object");
      const ELF32LE::Shdr &Sec = Sections[Sym.st_shndx];
      if (!(Sec.sh_flags & ELF::SHF_ALLOC) || !(Sec.sh_flags & AVMProgSpace) ||
          (Sec.sh_flags & (ELF::SHF_WRITE | ELF::SHF_EXECINSTR)))
        return bad(Twine("'") + ExceptionAnchorName +
                   "' must lie in a read-only AVM program-space section");
      if (Sym.st_size != 3)
        return bad(Twine("'") + ExceptionAnchorName +
                   "' must be exactly three bytes");
      if (Sym.st_value < Sec.sh_addr || Sec.sh_size < 3 ||
          Sym.st_value - Sec.sh_addr > Sec.sh_size - 3)
        return bad(Twine("'") + ExceptionAnchorName +
                   "' lies outside its containing section");
      if (Sym.st_value < HeaderSize)
        return bad(Twine("'") + ExceptionAnchorName +
                   "' overlaps the reserved header range");
      Anchor = static_cast<uint32_t>(Sym.st_value);
    }
  }
  return Anchor;
}

struct ExceptionRow {
  uint32_t Pc;                   // absolute address of the row
  uint8_t Flags;                 // saved mask in bits 3:0, r3 base in bit 7
  int16_t CFAOffset;
  SmallVector<int16_t, 4> Slots; // one per saved register, in r0-r3 order
};

struct ExceptionSite {
  uint32_t Start;
  uint32_t End;
  uint32_t LandingPad;
  uint8_t Action;
};

// One call in a function with landing pads. ReturnPc is the address after the
// call instruction, which is the saved return PC of the callee's frame.
struct ExceptionCall {
  uint32_t ReturnPc;
  uint16_t OutgoingBytes;
};

struct ExceptionFunction {
  uint32_t Start;
  uint32_t End;
  SmallVector<ExceptionRow, 8> Rows;
  SmallVector<ExceptionSite, 4> Sites;
  SmallVector<ExceptionCall, 4> Calls;
};

static bool sameRowState(const ExceptionRow &A, const ExceptionRow &B) {
  return A.Flags == B.Flags && A.CFAOffset == B.CFAOffset && A.Slots == B.Slots;
}

// Converts the effective CFI rows of one FDE. Only SP- or r3-based CFA rules
// and saved-register slots for r0-r3 are representable. Anything else is an
// error, because a silent default would unwind frames incorrectly.
static Expected<SmallVector<ExceptionRow, 8>>
decodeUnwindRows(const dwarf::FDE &Fde, uint32_t Start, uint32_t End,
                 StringRef InputName) {
  Expected<dwarf::UnwindTable> Table = createUnwindTable(&Fde);
  if (!Table)
    return bad(Twine("'") + InputName + "' has unsupported unwind information: " +
               toString(Table.takeError()));

  SmallVector<ExceptionRow, 8> Rows;
  for (const dwarf::UnwindRow &Row : *Table) {
    if (!Row.hasAddress())
      continue;
    uint64_t Address = Row.getAddress();
    if (Address < Start || Address >= End)
      return bad(Twine("unwind row at 0x") + Twine::utohexstr(Address) +
                 " lies outside its function");
    if (Address - Start > 0xffff)
      return bad(Twine("unwind row offset in function at 0x") +
                 Twine::utohexstr(Start) + " exceeds 16 bits");

    const dwarf::UnwindLocation &CFA = Row.getCFAValue();
    if (CFA.getLocation() != dwarf::UnwindLocation::RegPlusOffset ||
        (CFA.getRegister() != DwarfSP && CFA.getRegister() != DwarfR3))
      return bad(Twine("unsupported CFA rule at 0x") +
                 Twine::utohexstr(Address) + " in '" + InputName + "'");
    int32_t Offset = CFA.getOffset();
    if (Offset < INT16_MIN || Offset > INT16_MAX)
      return bad(Twine("CFA offset at 0x") + Twine::utohexstr(Address) +
                 " exceeds 16 bits");

    ExceptionRow Out;
    Out.Pc = static_cast<uint32_t>(Address);
    Out.Flags = CFA.getRegister() == DwarfR3 ? EHFlagR3Base : 0;
    Out.CFAOffset = static_cast<int16_t>(Offset);
    for (uint32_t R = 0; R < 4; ++R) {
      std::optional<dwarf::UnwindLocation> Loc =
          Row.getRegisterLocations().getRegisterLocation(R);
      if (!Loc || Loc->getLocation() == dwarf::UnwindLocation::Same ||
          Loc->getLocation() == dwarf::UnwindLocation::Unspecified)
        continue;
      if (Loc->getLocation() != dwarf::UnwindLocation::CFAPlusOffset)
        return bad(Twine("unsupported saved-register rule for r") + Twine(R) +
                   " at 0x" + Twine::utohexstr(Address));
      int32_t Slot = Loc->getOffset();
      if (Slot < INT16_MIN || Slot > INT16_MAX)
        return bad(Twine("saved-register offset for r") + Twine(R) +
                   " exceeds 16 bits");
      Out.Flags |= static_cast<uint8_t>(1u << R);
      Out.Slots.push_back(static_cast<int16_t>(Slot));
    }

    // A later row at the same address replaces the earlier one. Consecutive
    // rows with the same state need no record.
    if (!Rows.empty() && Rows.back().Pc == Out.Pc) {
      Rows.back() = Out;
      continue;
    }
    if (!Rows.empty() && sameRowState(Rows.back(), Out))
      continue;
    Rows.push_back(Out);
  }
  if (Rows.empty() || Rows.front().Pc != Start)
    return bad(Twine("'") + InputName + "' has no unwind row at function 0x" +
               Twine::utohexstr(Start));
  return Rows;
}

// Packs one record of .avm.eh_sites. Records are emitted only for functions
// with landing pads, so a missing record means no handler.
static Error appendSiteRecords(ArrayRef<uint8_t> Bytes, StringRef InputName,
                               SmallVectorImpl<ExceptionFunction> &Functions) {
  size_t Offset = 0;
  while (Offset < Bytes.size()) {
    if (Bytes.size() - Offset < EHSitesRecordHeaderSize)
      return bad(Twine("'") + InputName + "' has a truncated .avm.eh_sites record");
    uint32_t Start = Bytes[Offset] | (Bytes[Offset + 1] << 8) |
                     (Bytes[Offset + 2] << 16);
    uint32_t End = Bytes[Offset + 3] | (Bytes[Offset + 4] << 8) |
                   (Bytes[Offset + 5] << 16);
    uint16_t Count = Bytes[Offset + 6] | (Bytes[Offset + 7] << 8);
    Offset += EHSitesRecordHeaderSize;
    if (Bytes.size() - Offset < size_t(Count) * EHSiteSize)
      return bad(Twine("'") + InputName + "' has truncated .avm.eh_sites sites");

    ExceptionFunction *Fn = nullptr;
    for (ExceptionFunction &F : Functions)
      if (F.Start == Start)
        Fn = &F;
    if (!Fn || Fn->End != End)
      return bad(Twine("exception sites at 0x") + Twine::utohexstr(Start) +
                 " do not match a function with unwind information");

    for (uint16_t I = 0; I < Count; ++I, Offset += EHSiteSize) {
      const uint8_t *P = Bytes.data() + Offset;
      ExceptionSite Site;
      Site.Start = P[0] | (P[1] << 8) | (P[2] << 16);
      Site.End = P[3] | (P[4] << 8) | (P[5] << 16);
      Site.LandingPad = P[6] | (P[7] << 8) | (P[8] << 16);
      Site.Action = P[9];
      if (Site.Action == EHActionCleanup)
        return bad(Twine("cleanup landing pad in '") + InputName +
                   "' is unsupported: AVM exceptions handle only catch-all clauses");
      if (Site.Action > EHActionCatchAll)
        return bad(Twine("unknown exception action in '") + InputName + "'");
      Fn->Sites.push_back(Site);
    }
  }
  return Error::success();
}

// Packs one record of .avm.eh_calls: u24 function start, u16 count, then one
// u24 return PC and u16 outgoing byte count per call. The function end comes
// from its unwind entry, so the record does not repeat it.
static Error appendCallRecords(ArrayRef<uint8_t> Bytes, StringRef InputName,
                               SmallVectorImpl<ExceptionFunction> &Functions) {
  size_t Offset = 0;
  while (Offset < Bytes.size()) {
    if (Bytes.size() - Offset < EHCallsRecordHeaderSize)
      return bad(Twine("'") + InputName + "' has a truncated .avm.eh_calls record");
    uint32_t Start = Bytes[Offset] | (Bytes[Offset + 1] << 8) |
                     (Bytes[Offset + 2] << 16);
    uint16_t Count = Bytes[Offset + 3] | (Bytes[Offset + 4] << 8);
    Offset += EHCallsRecordHeaderSize;
    if (Bytes.size() - Offset < size_t(Count) * EHCallRecordSize)
      return bad(Twine("'") + InputName + "' has truncated .avm.eh_calls entries");

    ExceptionFunction *Fn = nullptr;
    for (ExceptionFunction &F : Functions)
      if (F.Start == Start)
        Fn = &F;
    if (!Fn)
      return bad(Twine("call records at 0x") + Twine::utohexstr(Start) +
                 " do not match a function with unwind information");
    if (!Fn->Calls.empty())
      return bad(Twine("duplicate call records for 0x") + Twine::utohexstr(Start));

    for (uint16_t I = 0; I < Count; ++I, Offset += EHCallRecordSize) {
      const uint8_t *P = Bytes.data() + Offset;
      ExceptionCall Call;
      Call.ReturnPc = P[0] | (P[1] << 8) | (P[2] << 16);
      Call.OutgoingBytes = P[3] | (P[4] << 8);
      Fn->Calls.push_back(Call);
    }
  }
  return Error::success();
}

// Returns the distance from a function record to the end of Out. The record's
// offset fields are 24 bits wide, so a larger distance is an error.
static Expected<uint32_t> recordOffset(const SmallVectorImpl<uint8_t> &Out,
                                       size_t Record, uint32_t Start) {
  size_t Offset = Out.size() - Record;
  if (Offset > 0xffffff)
    return bad(Twine("function at 0x") + Twine::utohexstr(Start) +
               " exceeds the 24-bit AVEH3 record offset");
  return static_cast<uint32_t>(Offset);
}

// The runtime locates the sites and calls from the two offsets, so the packer
// checks them before writing. The rows fill the bytes between the header and
// SitesOffset, and each row has five to thirteen bytes. Sites and calls fill the
// rest of the record exactly.
static Error checkRecordOffsets(const ExceptionFunction &Fn, uint32_t SitesOffset,
                                uint32_t NextOffset) {
  uint32_t Rows = Fn.Rows.size();
  if (SitesOffset < EHFunctionHeaderSize + EHRowHeaderSize * Rows ||
      SitesOffset > EHFunctionHeaderSize + EHRowMaxSize * Rows)
    return bad(Twine("function at 0x") + Twine::utohexstr(Fn.Start) +
               " has a sites offset outside its rows");
  uint32_t Body = EHSiteSize * Fn.Sites.size() + EHCallRecordSize * Fn.Calls.size();
  if (NextOffset < SitesOffset || NextOffset - SitesOffset != Body)
    return bad(Twine("function at 0x") + Twine::utohexstr(Fn.Start) +
               " has a next-record offset that does not match its sites and calls");
  return Error::success();
}

// Builds AVEH3 from the final ELF. Relaxation has already applied, so every
// address comes from .debug_frame or the relocated exception-record sections.
static Expected<SmallVector<uint8_t, 0>>
buildExceptionMetadata(const ELFFile<ELF32LE> &ELF,
                       ArrayRef<ELF32LE::Shdr> Sections,
                       ArrayRef<PayloadSection> Program, StringRef InputName) {
  const ELF32LE::Shdr *Frame = nullptr;
  const ELF32LE::Shdr *SiteSection = nullptr;
  const ELF32LE::Shdr *CallSection = nullptr;
  for (const ELF32LE::Shdr &S : Sections) {
    Expected<StringRef> NameOrErr = ELF.getSectionName(S);
    if (!NameOrErr)
      return NameOrErr.takeError();
    if (*NameOrErr == ".debug_frame")
      Frame = &S;
    else if (*NameOrErr == ".avm.eh_sites")
      SiteSection = &S;
    else if (*NameOrErr == ".avm.eh_calls")
      CallSection = &S;
  }
  if (!Frame)
    return bad(Twine("'") + InputName +
               "' references the exception runtime but has no .debug_frame "
               "unwind information");

  Expected<ArrayRef<uint8_t>> FrameBytes = contents(ELF, *Frame, ".debug_frame");
  if (!FrameBytes)
    return FrameBytes.takeError();
  DWARFDataExtractor Data(*FrameBytes, /*IsLittleEndian=*/true,
                          /*AddressSize=*/3);
  DWARFDebugFrame DebugFrame(Triple::avm, /*IsEH=*/false);
  if (Error E = DebugFrame.parse(Data))
    return bad(Twine("'") + InputName + "' has malformed .debug_frame: " +
               toString(std::move(E)));

  SmallVector<ExceptionFunction, 16> Functions;
  for (const dwarf::FrameEntry &Entry : DebugFrame.entries()) {
    const auto *Fde = dyn_cast<dwarf::FDE>(&Entry);
    if (!Fde)
      continue;
    uint64_t Start = Fde->getInitialLocation();
    uint64_t Range = Fde->getAddressRange();
    // A discarded function keeps its FDE, but the linker resolves it to zero.
    if (Start == 0)
      continue;
    const PayloadSection *Code = nullptr;
    for (const PayloadSection &S : Program)
      if ((S.Flags & ELF::SHF_EXECINSTR) && Start >= S.Address &&
          Start < S.Address + S.Contents.size())
        Code = &S;
    if (!Code)
      return bad(Twine("unwind entry at 0x") + Twine::utohexstr(Start) +
                 " does not describe executable program code");
    if (Range == 0 || Start + Range > Code->Address + Code->Contents.size())
      return bad(Twine("unwind entry at 0x") + Twine::utohexstr(Start) +
                 " has an invalid address range");

    ExceptionFunction Fn;
    Fn.Start = static_cast<uint32_t>(Start);
    Fn.End = static_cast<uint32_t>(Start + Range);
    Expected<SmallVector<ExceptionRow, 8>> Rows =
        decodeUnwindRows(*Fde, Fn.Start, Fn.End, InputName);
    if (!Rows)
      return Rows.takeError();
    Fn.Rows = std::move(*Rows);
    Functions.push_back(std::move(Fn));
  }
  llvm::sort(Functions, [](const ExceptionFunction &A,
                           const ExceptionFunction &B) {
    return A.Start < B.Start;
  });
  for (size_t I = 1; I < Functions.size(); ++I)
    if (Functions[I].Start < Functions[I - 1].End)
      return bad(Twine("overlapping unwind ranges at 0x") +
                 Twine::utohexstr(Functions[I].Start));

  if (SiteSection) {
    Expected<ArrayRef<uint8_t>> SiteBytes =
        contents(ELF, *SiteSection, ".avm.eh_sites");
    if (!SiteBytes)
      return SiteBytes.takeError();
    if (Error E = appendSiteRecords(*SiteBytes, InputName, Functions))
      return std::move(E);
  }
  if (CallSection) {
    Expected<ArrayRef<uint8_t>> CallBytes =
        contents(ELF, *CallSection, ".avm.eh_calls");
    if (!CallBytes)
      return CallBytes.takeError();
    if (Error E = appendCallRecords(*CallBytes, InputName, Functions))
      return std::move(E);
  }

  // The site lists are emitted in address order, and every landing pad must
  // lie inside its function. These checks match the runtime's validation.
  for (ExceptionFunction &Fn : Functions) {
    llvm::sort(Fn.Calls, [](const ExceptionCall &A, const ExceptionCall &B) {
      return A.ReturnPc < B.ReturnPc;
    });
    for (size_t I = 0; I < Fn.Calls.size(); ++I) {
      const ExceptionCall &Call = Fn.Calls[I];
      // Each return PC follows its call instruction, so it lies in (Start, End].
      if (Call.ReturnPc <= Fn.Start || Call.ReturnPc > Fn.End)
        return bad(Twine("call return PC 0x") + Twine::utohexstr(Call.ReturnPc) +
                   " lies outside its function");
      if (I > 0 && Fn.Calls[I - 1].ReturnPc == Call.ReturnPc)
        return bad(Twine("duplicate call record at 0x") +
                   Twine::utohexstr(Call.ReturnPc));
      if (Call.OutgoingBytes >= llvm::AVM::StackLimit)
        return bad(Twine("call at 0x") + Twine::utohexstr(Call.ReturnPc) +
                   " passes more outgoing bytes than the AVM stack holds");
    }
    llvm::sort(Fn.Sites, [](const ExceptionSite &A, const ExceptionSite &B) {
      return A.Start < B.Start;
    });
    uint32_t PreviousEnd = Fn.Start;
    for (const ExceptionSite &Site : Fn.Sites) {
      if (Site.Start >= Site.End || Site.Start < PreviousEnd || Site.End > Fn.End)
        return bad(Twine("invalid exception site at 0x") +
                   Twine::utohexstr(Site.Start));
      if (Site.Action == EHActionNone) {
        if (Site.LandingPad != 0)
          return bad(Twine("exception site at 0x") +
                     Twine::utohexstr(Site.Start) + " has a landing pad but no handler");
      } else if (Site.LandingPad < Fn.Start || Site.LandingPad >= Fn.End) {
        return bad(Twine("landing pad 0x") + Twine::utohexstr(Site.LandingPad) +
                   " lies outside its function");
      } else {
        // The invoke's call returns inside its site. The landing pad releases
        // that call's stack arguments, so the call needs a record.
        bool HasCall = llvm::any_of(Fn.Calls, [&](const ExceptionCall &Call) {
          return Call.ReturnPc > Site.Start && Call.ReturnPc <= Site.End;
        });
        if (!HasCall)
          return bad(Twine("catch-all site at 0x") + Twine::utohexstr(Site.Start) +
                     " has no call record for its outgoing arguments");
      }
      PreviousEnd = Site.End;
    }
  }

  if (Functions.size() > 0xffff)
    return bad("AVEH3 exceeds 65535 functions");
  SmallVector<uint8_t, 0> Out;
  for (char C : StringRef("AVEH3"))
    Out.push_back(static_cast<uint8_t>(C));
  append16(Out, static_cast<uint16_t>(Functions.size()));
  append24Value(Out, 0); // block size, patched below
  for (const ExceptionFunction &Fn : Functions) {
    if (Fn.Rows.size() > 0xffff || Fn.Sites.size() > 0xffff ||
        Fn.Calls.size() > 0xffff)
      return bad(Twine("function at 0x") + Twine::utohexstr(Fn.Start) +
                 " exceeds AVEH3 record limits");
    size_t Record = Out.size();
    append24Value(Out, Fn.Start);
    append24Value(Out, Fn.End);
    append16(Out, static_cast<uint16_t>(Fn.Rows.size()));
    append16(Out, static_cast<uint16_t>(Fn.Sites.size()));
    append16(Out, static_cast<uint16_t>(Fn.Calls.size()));
    // The two offsets are measured from Record. They are patched after the
    // rows, sites, and calls that they bound are written.
    size_t SitesOffsetField = Out.size();
    append24Value(Out, 0);
    size_t NextOffsetField = Out.size();
    append24Value(Out, 0);
    for (const ExceptionRow &Row : Fn.Rows) {
      append16(Out, static_cast<uint16_t>(Row.Pc - Fn.Start));
      Out.push_back(Row.Flags);
      append16(Out, static_cast<uint16_t>(Row.CFAOffset));
      for (int16_t Slot : Row.Slots)
        append16(Out, static_cast<uint16_t>(Slot));
    }
    Expected<uint32_t> SitesOffset = recordOffset(Out, Record, Fn.Start);
    if (!SitesOffset)
      return SitesOffset.takeError();
    for (const ExceptionSite &Site : Fn.Sites) {
      append24Value(Out, Site.Start);
      append24Value(Out, Site.End);
      append24Value(Out, Site.LandingPad);
      Out.push_back(Site.Action);
    }
    for (const ExceptionCall &Call : Fn.Calls) {
      append24Value(Out, Call.ReturnPc);
      append16(Out, Call.OutgoingBytes);
    }
    Expected<uint32_t> NextOffset = recordOffset(Out, Record, Fn.Start);
    if (!NextOffset)
      return NextOffset.takeError();
    if (Error E = checkRecordOffsets(Fn, *SitesOffset, *NextOffset))
      return std::move(E);
    write24(Out, SitesOffsetField, *SitesOffset);
    write24(Out, NextOffsetField, *NextOffset);
  }
  if (Out.size() > MaxFlashSize)
    return bad("AVEH3 metadata exceeds the 24-bit block size");
  write24(Out, 7, static_cast<uint32_t>(Out.size()));
  return Out;
}

static Error packageELF(StringRef InputName, StringRef DefaultInterpreter) {
  Expected<OwningBinary<ObjectFile>> Binary = ObjectFile::createObjectFile(Input);
  if (!Binary)
    return createFileError(InputName, Binary.takeError());
  auto *ELFObj = dyn_cast<ELF32LEObjectFile>(Binary->getBinary());
  if (!ELFObj)
    return bad(Twine("'") + InputName + "' is not a little-endian ELF32 file");

  const ELFFile<ELF32LE> &ELF = ELFObj->getELFFile();
  const ELF32LE::Ehdr &H = ELF.getHeader();
  if (H.e_ident[ELF::EI_CLASS] != ELF::ELFCLASS32 ||
      H.e_ident[ELF::EI_DATA] != ELF::ELFDATA2LSB ||
      H.e_ident[ELF::EI_VERSION] != ELF::EV_CURRENT ||
      H.e_ident[ELF::EI_OSABI] != ELF::ELFOSABI_NONE ||
      H.e_ident[ELF::EI_ABIVERSION] != 0)
    return bad(Twine("'") + InputName + "' must be ELFCLASS32 and ELFDATA2LSB");
  if (H.e_type != ELF::ET_EXEC || H.e_version != ELF::EV_CURRENT)
    return bad(Twine("'") + InputName + "' must be a linked AVM ET_EXEC");
  if (H.e_machine != 0x4156 || H.e_flags != ELF::EF_AVM_ABI_V2)
    return bad(Twine("'") + InputName + "' has unsupported AVM ELF identity");
  Expected<uint32_t> PhNum = ELF.getPhNum();
  if (!PhNum)
    return PhNum.takeError();
  if (*PhNum != 0)
    return bad(Twine("'") + InputName + "' has unsupported program headers");

  Expected<ArrayRef<ELF32LE::Shdr>> SectionsOrErr = ELF.sections();
  if (!SectionsOrErr)
    return SectionsOrErr.takeError();

  const ELF32LE::Shdr *Saved = nullptr;
  const ELF32LE::Shdr *Data = nullptr;
  SmallVector<PayloadSection, 8> Program;
  for (const ELF32LE::Shdr &S : *SectionsOrErr) {
    Expected<StringRef> NameOrErr = ELF.getSectionName(S);
    if (!NameOrErr)
      return NameOrErr.takeError();
    StringRef Name = *NameOrErr;
    bool Alloc = S.sh_flags & ELF::SHF_ALLOC;
    bool Prog = S.sh_flags & AVMProgSpace;
    bool DataSpace = S.sh_flags & AVMDataSpace;
    if (Alloc && Prog == DataSpace)
      return bad(Twine("allocated AVM section '") + Name +
                 "' must have exactly one AVM address-space flag");
    if (Alloc && S.sh_type == ELF::SHT_NOBITS)
      return bad(Twine("allocated AVM section '") + Name +
                 "' must not use SHT_NOBITS (.bss is unsupported)");
    if (Alloc && (S.sh_flags & ELF::SHF_TLS))
      return bad(Twine("allocated AVM section '") + Name + "' uses TLS");
    if (S.sh_type == ELF::SHT_DYNAMIC || S.sh_type == ELF::SHT_DYNSYM)
      return bad(Twine("'") + InputName + "' uses dynamic linking");
    if (Alloc && (S.sh_flags & ELF::SHF_COMPRESSED))
      return bad(Twine("allocated AVM section '") + Name + "' is compressed");
    if (Alloc && (S.sh_type == ELF::SHT_REL || S.sh_type == ELF::SHT_RELA))
      return bad(Twine("allocated relocation section '") + Name + "' is unsupported");
    if (S.sh_type == ELF::SHT_SYMTAB) {
      Expected<ArrayRef<ELF32LE::Sym>> Symbols =
          ELF.getSectionContentsAsArray<ELF32LE::Sym>(S);
      if (!Symbols)
        return Symbols.takeError();
      for (const ELF32LE::Sym &Sym : *Symbols)
        if (Sym.st_shndx == ELF::SHN_UNDEF &&
            Sym.getBinding() != ELF::STB_LOCAL &&
            Sym.getBinding() != ELF::STB_WEAK)
          return bad(Twine("'") + InputName + "' has an undefined symbol");
    }
    if (!Alloc)
      continue;

    if (DataSpace) {
      if (Name != ".saved" && Name != ".data")
        return bad(Twine("unsupported allocated data-space section '") + Name + "'");
      if (S.sh_type != ELF::SHT_PROGBITS ||
          (S.sh_flags & (ELF::SHF_WRITE | ELF::SHF_EXECINSTR)) != ELF::SHF_WRITE)
        return bad(Twine("AVM data section '") + Name + "' has invalid type or flags");
      if (Name == ".saved") {
        if (Saved)
          return bad("multiple .saved output sections are unsupported");
        Saved = &S;
      } else {
        if (Data)
          return bad("multiple .data output sections are unsupported");
        Data = &S;
      }
      continue;
    }

    if (S.sh_flags & ELF::SHF_WRITE)
      return bad(Twine("AVM program-space section '") + Name + "' is writable");
    if (S.sh_type != ELF::SHT_PROGBITS && S.sh_type != ELF::SHT_INIT_ARRAY &&
        S.sh_type != ELF::SHT_FINI_ARRAY)
      return bad(Twine("unsupported allocated program-space section '") + Name + "'");
    Expected<ArrayRef<uint8_t>> Bytes = contents(ELF, S, Name);
    if (!Bytes)
      return Bytes.takeError();
    Expected<uint32_t> End = checkedEnd(S.sh_addr, S.sh_size, Name);
    if (!End)
      return End.takeError();
    if (S.sh_addr < HeaderSize && S.sh_size)
      return bad(Twine("AVM program-space section '") + Name +
                 "' overlaps the reserved header range");
    Program.push_back({Name, S.sh_addr, *Bytes, S.sh_flags});
  }

  uint32_t SaveSize = Saved ? Saved->sh_size : 0;
  uint32_t DataSuffixSize = Data ? Data->sh_size : 0;
  if (SaveSize > MaxDataSize || DataSuffixSize > MaxDataSize - SaveSize)
    return bad(".saved + .data exceeds the 1024-byte AVM static-storage limit");
  uint32_t DataSize = SaveSize + DataSuffixSize;
  if (Saved && Saved->sh_addr != HeaderSize)
    return bad(".saved must begin at data address 0x100");
  if (Data && Data->sh_addr != HeaderSize + SaveSize)
    return bad(".data must immediately follow .saved at its AVM data address");
  Expected<ArrayRef<uint8_t>> SavedBytes =
      Saved ? contents(ELF, *Saved, ".saved") : ArrayRef<uint8_t>();
  if (!SavedBytes)
    return SavedBytes.takeError();
  Expected<ArrayRef<uint8_t>> DataBytes =
      Data ? contents(ELF, *Data, ".data") : ArrayRef<uint8_t>();
  if (!DataBytes)
    return DataBytes.takeError();

  uint32_t ProgramStart = alignTo(HeaderSize + DataSize, PageSize);
  uint32_t PayloadEnd = HeaderSize + DataSize;
  bool EntryIsLiveCode = false;
  for (const PayloadSection &S : Program) {
    if (S.Address < ProgramStart)
      return bad(Twine("AVM program-space section '") + S.Name +
                 "' begins before programStart");
    Expected<uint32_t> End = checkedEnd(S.Address, S.Contents.size(), S.Name);
    if (!End)
      return End.takeError();
    PayloadEnd = std::max(PayloadEnd, *End);
    if ((S.Flags & ELF::SHF_EXECINSTR) && H.e_entry >= S.Address &&
        H.e_entry < *End)
      EntryIsLiveCode = true;
  }
  if (Error E = checkNoOverlap(Program, HeaderSize + DataSize))
    return E;
  if (H.e_entry > MaxProgramAddress || H.e_entry < HeaderSize ||
      !EntryIsLiveCode)
    return bad("ELF entry point is not a byte in an executable AVM program-space section");

  Expected<uint32_t> TrapEntry = findTrapEntry(ELF, *SectionsOrErr, InputName);
  if (!TrapEntry)
    return TrapEntry.takeError();

  // Crash metadata is included by default and follows the payload at a 4-byte
  // boundary. It is derived from the final ELF, so its addresses already
  // include relaxation. --no-crash-info skips the inspection entirely.
  uint64_t CrashStart = alignTo(uint64_t(PayloadEnd), 4);
  uint64_t Room = CrashStart <= MaxPayloadEnd ? MaxPayloadEnd - CrashStart : 0;
  SmallVector<uint8_t, 0> CrashBlock;
  if (!NoCrashInfo)
    CrashBlock =
        buildCrashInfo(*ELFObj, ELF, *SectionsOrErr, InputName, Room);
  uint64_t FinalPayloadEnd = PayloadEnd;
  if (!CrashBlock.empty())
    FinalPayloadEnd = CrashStart + CrashBlock.size();
  else
    CrashStart = 0;

  // AVEH3 follows the crash metadata, and only programs that reference the
  // exception runtime define the anchor. --no-crash-info does not affect it.
  Expected<std::optional<uint32_t>> ExceptionAnchor =
      findExceptionAnchor(ELF, *SectionsOrErr, InputName);
  if (!ExceptionAnchor)
    return ExceptionAnchor.takeError();
  std::optional<uint32_t> AnchorAddress = *ExceptionAnchor;
  SmallVector<uint8_t, 0> ExceptionBlock;
  uint64_t ExceptionStart = 0;
  if (AnchorAddress) {
    Expected<SmallVector<uint8_t, 0>> Block =
        buildExceptionMetadata(ELF, *SectionsOrErr, Program, InputName);
    if (!Block)
      return Block.takeError();
    ExceptionBlock = std::move(*Block);
    ExceptionStart = alignTo(FinalPayloadEnd, 4);
    if (ExceptionStart + ExceptionBlock.size() > MaxPayloadEnd)
      return bad("AVEH3 exception metadata exceeds the flash payload limit");
    FinalPayloadEnd = ExceptionStart + ExceptionBlock.size();
  }

  Expected<uint32_t> FileSize =
      finalFileSize(static_cast<uint32_t>(FinalPayloadEnd));
  if (!FileSize)
    return FileSize.takeError();
  bool Arduboy = StringRef(Output).ends_with_insensitive(".arduboy");
  if (((Arduboy && SaveSize) || (!Arduboy && Development)) &&
      *FileSize > MaxFlashSize - DevelopmentSaveSize)
    return bad("AVM image leaves no room for a 4 KiB save area");
  SmallVector<uint8_t, 0> Image(*FileSize, 0xff);
  std::fill(Image.begin(), Image.begin() + HeaderSize, 0);
  Image[0] = 0x41; Image[1] = 0x56; Image[2] = 0x4d; Image[3] = 0x01;
  Image[4] = RuntimeVersion;
  write24(Image, 5, H.e_entry);
  write16le(Image.data() + 8, DataSize);
  write16le(Image.data() + 10, SaveSize);
  write24(Image, 0x0c, *TrapEntry);
  // Zero means the image has no crash metadata. The reserved bytes stay zero.
  write24(Image, CrashInfoOffset, static_cast<uint32_t>(CrashStart));
  llvm::copy(*SavedBytes, Image.begin() + HeaderSize);
  llvm::copy(*DataBytes, Image.begin() + HeaderSize + SaveSize);
  for (const PayloadSection &S : Program)
    llvm::copy(S.Contents, Image.begin() + S.Address);
  if (!CrashBlock.empty())
    llvm::copy(CrashBlock, Image.begin() + CrashStart);
  if (AnchorAddress) {
    llvm::copy(ExceptionBlock, Image.begin() + ExceptionStart);
    write24(Image, *AnchorAddress, static_cast<uint32_t>(ExceptionStart));
  }
  size_t Tail = Image.size() - 8;
  Image[Tail] = 0x41; Image[Tail + 1] = 0x56;
  Image[Tail + 2] = 0x54; Image[Tail + 3] = 0x01;
  write16le(Image.data() + Tail + 4, *FileSize / PageSize);
  write16le(Image.data() + Tail + 6, 0);
  write32le(Image.data() + 0xfc, crc32(ArrayRef<uint8_t>(Image).take_front(0xfc)));

  if (Arduboy) {
    StringRef InterpreterPath = Interpreter.empty() ? DefaultInterpreter
                                                    : StringRef(Interpreter);
    ErrorOr<std::unique_ptr<MemoryBuffer>> Hex = MemoryBuffer::getFile(InterpreterPath);
    if (!Hex)
      return createFileError(InterpreterPath, Hex.getError());
    StringRef HexBytes = (*Hex)->getBuffer();
    if (HexBytes.empty())
      return bad("interpreter HEX is empty");
    std::string GameTitle = Title.empty() ? sys::path::stem(InputName).str()
                                          : Title;
    if (GameTitle.empty() || Author.empty() || Version.empty())
      return bad(".arduboy title, author, and game version must not be empty");
    std::string Info = "{\"schemaVersion\":3,\"title\":" +
                       jsonString(GameTitle) + ",\"author\":" +
                       jsonString(Author) + ",\"version\":" +
                       jsonString(Version);
    if (!Description.empty())
      Info += ",\"description\":" + jsonString(Description);
    if (!Genre.empty())
      Info += ",\"genre\":" + jsonString(Genre);
    Info += ",\"binaries\":[{\"filename\":\"interp.hex\",\"flashdata\":"
            "\"fxdata.bin\"";
    if (SaveSize)
      Info += ",\"flashsave\":\"fxsave.bin\"";
    Info += ",\"device\":\"ArduboyFX\"}]}\n";
    SmallVector<uint8_t, DevelopmentSaveSize> Save(DevelopmentSaveSize, 0xff);
    SmallVector<ZipEntry, 4> Entries = {
        {"info.json", ArrayRef<uint8_t>(
                          reinterpret_cast<const uint8_t *>(Info.data()),
                          Info.size())},
        {"interp.hex", ArrayRef<uint8_t>(
                           reinterpret_cast<const uint8_t *>(HexBytes.data()),
                           HexBytes.size())},
        {"fxdata.bin", Image},
    };
    if (SaveSize)
      Entries.push_back({"fxsave.bin", Save});
    Image = makeZip(Entries);
  } else {
    if (!Interpreter.empty() || !Title.empty() || !Description.empty() ||
        !Genre.empty() || Author.getNumOccurrences() ||
        Version.getNumOccurrences())
      return bad(".arduboy metadata options require a .arduboy output file");
    // Keep the AVM tail and its page count tied to the executable image. The
    // optional save sector is raw erased flash outside the flat image.
    if (Development)
      Image.resize(Image.size() + DevelopmentSaveSize, 0xff);
  }

  Expected<std::unique_ptr<FileOutputBuffer>> Buffer =
      FileOutputBuffer::create(Output, Image.size());
  if (!Buffer)
    return Buffer.takeError();
  llvm::copy(Image, (*Buffer)->getBufferStart());
  return (*Buffer)->commit();
}
} // namespace

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv, "AVM ET_EXEC flat-image packer\n");
  SmallString<256> DefaultInterpreter(
      sys::fs::getMainExecutable(argv[0], nullptr));
  sys::path::remove_filename(DefaultInterpreter);
  sys::path::append(DefaultInterpreter, "avm", "interp.hex");
  if (Error E = packageELF(Input, DefaultInterpreter)) {
    WithColor::error(errs(), "llvm-avm-image") << Input << ": "
                                                 << toString(std::move(E))
                                                 << '\n';
    return 1;
  }
  return 0;
}
