//===-- llvm-avm-image.cpp - AVM flat image packer -----------------------===//

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
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
constexpr uint32_t CrashHeaderSize = 24;
constexpr uint32_t CrashFunctionSize = 9;
constexpr uint32_t CrashLineSize = 12;
constexpr uint32_t CrashMaxRecords = 0xffff;
constexpr size_t CrashMaxNameBytes = 80;
constexpr size_t CrashMaxFileBytes = 48;

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

static std::string crashName(StringRef RawName) {
  std::string Name = sanitizeCrashText(demangle(RawName));
  if (Name.size() > CrashMaxNameBytes)
    Name.resize(CrashMaxNameBytes);
  return Name;
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
      Candidates.push_back({Sym.st_value, static_cast<uint32_t>(SectionEnd),
                            Sym.st_size, Sym.getBinding() != ELF::STB_LOCAL,
                            Name.starts_with(".L") || Name.starts_with("$"),
                            crashName(Name)});
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

// Encodes the AVD1 block in the order header, function table, line table,
// string table. Strings are deduplicated in record order.
static Expected<SmallVector<uint8_t, 0>>
encodeCrashInfo(ArrayRef<CodeRange> Functions, ArrayRef<CodeRange> Lines) {
  if (Functions.size() > CrashMaxRecords || Lines.size() > CrashMaxRecords)
    return bad("crash metadata exceeds 65535 function or source-line records");
  uint64_t FunctionTable = CrashHeaderSize;
  uint64_t LineTable = FunctionTable + Functions.size() * CrashFunctionSize;
  uint64_t StringTable = LineTable + Lines.size() * CrashLineSize;

  SmallVector<uint8_t, 0> Strings;
  StringMap<uint64_t> StringOffsets;
  auto intern = [&](StringRef Text) -> uint64_t {
    auto [It, Inserted] = StringOffsets.try_emplace(Text, Strings.size());
    if (Inserted) {
      llvm::append_range(Strings, Text.bytes());
      Strings.push_back(0);
    }
    return StringTable + It->getValue();
  };

  SmallVector<uint8_t, 0> Out;
  Out.append({'A', 'V', 'D', '1'});
  append24Value(Out, 0); // Total size, written after the tables.
  Out.push_back(0);      // Flags.
  append16(Out, Functions.size());
  append16(Out, Lines.size());
  append24Value(Out, FunctionTable);
  append24Value(Out, LineTable);
  append24Value(Out, StringTable);
  append24Value(Out, 0); // Reserved.

  for (const CodeRange &F : Functions) {
    append24Value(Out, F.Start);
    append24Value(Out, F.End);
    append24Value(Out, intern(F.Text));
  }
  for (const CodeRange &L : Lines) {
    append24Value(Out, L.Start);
    append24Value(Out, L.End);
    append24Value(Out, intern(L.Text));
    append24Value(Out, L.Line);
  }

  uint64_t Size = StringTable + Strings.size();
  if (Size > 0xffffff)
    return bad("crash metadata exceeds the 24-bit AVD1 size field");
  llvm::append_range(Out, Strings);
  write24(Out, 4, static_cast<uint32_t>(Size));
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

// Returns an empty block when no usable function or source-line data remains.
static SmallVector<uint8_t, 0> buildCrashInfo(const ObjectFile &Obj,
                                              const ELFFile<ELF32LE> &ELF,
                                              ArrayRef<ELF32LE::Shdr> Sections,
                                              StringRef InputName) {
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
  if (!Block) {
    warnCrashInfo(InputName, "crash metadata", Block.takeError());
    return {};
  }
  return std::move(*Block);
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
  SmallVector<uint8_t, 0> CrashBlock;
  if (!NoCrashInfo)
    CrashBlock = buildCrashInfo(*ELFObj, ELF, *SectionsOrErr, InputName);
  uint64_t CrashStart = 0;
  uint64_t FinalPayloadEnd = PayloadEnd;
  if (!CrashBlock.empty()) {
    uint64_t Start = alignTo(uint64_t(PayloadEnd), 4);
    uint64_t End = Start + CrashBlock.size();
    if (Start > MaxProgramAddress || End > MaxPayloadEnd) {
      warnCrashInfo(InputName, "crash metadata",
                    bad("it does not fit in the Version 1 flat image"));
      CrashBlock.clear();
    } else {
      CrashStart = Start;
      FinalPayloadEnd = End;
    }
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
