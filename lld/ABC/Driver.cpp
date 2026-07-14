//===- ABC/Driver.cpp -----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lld/Common/CommonLinkerContext.h"
#include "lld/Common/Driver.h"
#include "lld/Common/ErrorHandler.h"
#include "lld/Common/Version.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/Binary.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileOutputBuffer.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/TargetParser/Triple.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace llvm;
using namespace llvm::object;

namespace lld::abc {
namespace {

constexpr uint8_t ABC_SIGNATURE[] = {0xAB, 0xC0, 0x0A, 0xBC};
constexpr uint8_t ABC_END_SIGNATURE[] = {0xAB, 0xCE, 0xEA, 0xBC};
constexpr uint8_t ABC_OP_ALLOC = 0x4A;
constexpr uint8_t ABC_OP_JMP = 0xB1;
constexpr uint8_t ABC_OP_JMP1 = 0xB2;
constexpr uint8_t ABC_OP_CALL = 0xB5;
constexpr uint8_t ABC_OP_RET = 0xB9;
constexpr StringLiteral ABC_MAIN_RETURN_BYTES_SYMBOL = "$__abc.main.retbytes";

struct SectionChunk {
  std::string Name;
  uint64_t ObjectIndex = 0;
  uint64_t SectionIndex = 0;
  uint64_t OutputOffset = 0;
  bool IsInitStubSection = false;
  std::vector<uint8_t> Contents;

  struct Reloc {
    uint64_t Offset = 0;
    uint32_t Type = 0;
    std::string SymbolName;
    std::optional<uint64_t> TargetObjectIndex;
    std::optional<uint64_t> TargetSectionIndex;
    uint64_t TargetSymbolValue = 0;
    int64_t Addend = 0;
  };
  std::vector<Reloc> Relocs;
};

struct SymbolDef {
  std::string Name;
  uint64_t ObjectIndex = 0;
  std::optional<uint64_t> SectionIndex;
  uint64_t Value = 0;
  bool Absolute = false;
};

struct RamSection {
  std::string Name;
  uint64_t ObjectIndex = 0;
  uint64_t SectionIndex = 0;
  uint64_t OutputOffset = 0;
  uint64_t Size = 0;
  bool Saved = false;
};

struct InputState {
  std::vector<SectionChunk> ProgramData;
  std::vector<SectionChunk> Text;
  std::vector<RamSection> Ram;
  std::vector<SymbolDef> Symbols;
  uint64_t SavedBytes = 0;
  uint64_t BssBytes = 0;
  uint64_t NextObjectIndex = 0;
};

struct Config {
  std::string Output = "fxdata.bin";
  std::vector<std::string> Inputs;
  uint8_t Shades = 2;
  std::optional<uint16_t> SaveSize;
};

static void set16(std::vector<uint8_t> &Buf, size_t Offset, uint16_t Value) {
  Buf[Offset] = Value & 0xff;
  Buf[Offset + 1] = (Value >> 8) & 0xff;
}

static void set24(std::vector<uint8_t> &Buf, size_t Offset, uint32_t Value) {
  Buf[Offset] = Value & 0xff;
  Buf[Offset + 1] = (Value >> 8) & 0xff;
  Buf[Offset + 2] = (Value >> 16) & 0xff;
}

static void append24(std::vector<uint8_t> &Buf, uint32_t Value) {
  Buf.push_back(Value & 0xff);
  Buf.push_back((Value >> 8) & 0xff);
  Buf.push_back((Value >> 16) & 0xff);
}

static void writeLittle(std::vector<uint8_t> &Buf, uint64_t Offset,
                        uint64_t Value, unsigned Width) {
  for (unsigned I = 0; I < Width; ++I)
    Buf[Offset + I] = uint8_t(Value >> (I * 8));
}

static bool parseUInt(StringRef Text, uint64_t &Value) {
  return !Text.getAsInteger(0, Value);
}

static void printHelp(raw_ostream &OS, StringRef Argv0) {
  OS << "OVERVIEW: ABC VM linker\n\n";
  OS << "USAGE: " << Argv0 << " [options] file...\n\n";
  OS << "OPTIONS:\n";
  OS << "  -o <path>          Write ABC VM binary to <path>\n";
  OS << "  --shades <n>       Set display shade count, normally 2, 3, or 4\n";
  OS << "  --save-size <n>    Override saved-global byte count\n";
  OS << "  --help             Display this help\n";
}

static std::optional<Config> parseArgs(ArrayRef<const char *> Args,
                                       raw_ostream &StdoutOS) {
  Config C;
  for (size_t I = 1; I < Args.size(); ++I) {
    StringRef Arg = Args[I];
    auto NeedValue = [&](StringRef Opt) -> std::optional<StringRef> {
      if (I + 1 == Args.size()) {
        lld::error(Opt + ": missing argument");
        return std::nullopt;
      }
      return StringRef(Args[++I]);
    };

    if (Arg == "--help" || Arg == "-help" || Arg == "-h") {
      printHelp(StdoutOS, Args[0]);
      return std::nullopt;
    }
    if (Arg == "--version" || Arg == "-v") {
      StdoutOS << getLLDVersion() << "\n";
      return std::nullopt;
    }
    if (Arg == "-o") {
      if (std::optional<StringRef> Value = NeedValue(Arg))
        C.Output = std::string(*Value);
      continue;
    }
    if (Arg.consume_front("-o") && !Arg.empty()) {
      C.Output = std::string(Arg);
      continue;
    }
    if (Arg == "--shades") {
      if (std::optional<StringRef> Value = NeedValue(Arg)) {
        uint64_t Parsed = 0;
        if (!parseUInt(*Value, Parsed) || Parsed > 255)
          lld::error("--shades: expected an 8-bit integer");
        else
          C.Shades = Parsed;
      }
      continue;
    }
    if (Arg.consume_front("--shades=")) {
      uint64_t Parsed = 0;
      if (!parseUInt(Arg, Parsed) || Parsed > 255)
        lld::error("--shades: expected an 8-bit integer");
      else
        C.Shades = Parsed;
      continue;
    }
    if (Arg == "--save-size") {
      if (std::optional<StringRef> Value = NeedValue(Arg)) {
        uint64_t Parsed = 0;
        if (!parseUInt(*Value, Parsed) || Parsed > UINT16_MAX)
          lld::error("--save-size: expected a 16-bit integer");
        else
          C.SaveSize = static_cast<uint16_t>(Parsed);
      }
      continue;
    }
    if (Arg.consume_front("--save-size=")) {
      uint64_t Parsed = 0;
      if (!parseUInt(Arg, Parsed) || Parsed > UINT16_MAX)
        lld::error("--save-size: expected a 16-bit integer");
      else
        C.SaveSize = static_cast<uint16_t>(Parsed);
      continue;
    }
    if (Arg == "-L" || Arg == "-u") {
      (void)NeedValue(Arg);
      continue;
    }
    if (Arg.starts_with("-L") || Arg.starts_with("-u"))
      continue;
    if (Arg.starts_with("-")) {
      lld::error("unknown argument: " + Arg);
      continue;
    }
    C.Inputs.push_back(std::string(Arg));
  }

  if (C.Inputs.empty())
    lld::error("no input files");
  if (lld::errorCount())
    return std::nullopt;
  return C;
}

static unsigned relocWidth(uint32_t Type) {
  switch (Type) {
  case ELF::R_ABC_8:
  case ELF::R_ABC_GLOBAL8:
  case ELF::R_ABC_BRANCH8:
    return 1;
  case ELF::R_ABC_16:
  case ELF::R_ABC_GLOBAL16_TAGGED:
  case ELF::R_ABC_BRANCH16:
    return 2;
  case ELF::R_ABC_24:
  case ELF::R_ABC_PROG24:
  case ELF::R_ABC_CALL24:
    return 3;
  case ELF::R_ABC_32:
    return 4;
  default:
    return 0;
  }
}

static SectionChunk makeSection(uint64_t ObjIndex, const SectionRef &Sec,
                                StringRef Name, StringRef Contents) {
  SectionChunk Chunk;
  Chunk.Name = std::string(Name);
  Chunk.ObjectIndex = ObjIndex;
  Chunk.SectionIndex = Sec.getIndex();
  Chunk.IsInitStubSection = Name.starts_with(".text.$globinit.");
  Chunk.Contents.assign(Contents.bytes_begin(), Contents.bytes_end());
  return Chunk;
}

static void readRelocations(ObjectFile &Obj, uint64_t ObjIndex,
                            const SectionRef &RelSec, StringRef Path,
                            SectionChunk &Chunk) {
  for (const RelocationRef &Rel : RelSec.relocations()) {
    uint32_t Type = Rel.getType();
    unsigned Width = relocWidth(Type);
    if (!Width) {
      lld::error(Path + ": unsupported ABC relocation type " +
                 Twine(Type).str());
      continue;
    }

    uint64_t Offset = Rel.getOffset();
    if (Offset + Width > Chunk.Contents.size()) {
      lld::error(Path + ": relocation offset is outside section " +
                 Chunk.Name);
      continue;
    }

    symbol_iterator Sym = Rel.getSymbol();
    if (Sym == Obj.symbol_end()) {
      lld::error(Path + ": relocation without a symbol");
      continue;
    }

    Expected<StringRef> NameOrErr = Sym->getName();
    if (!NameOrErr) {
      lld::error(Path + ": " + toString(NameOrErr.takeError()));
      continue;
    }

    std::optional<uint64_t> TargetSectionIndex;
    uint64_t TargetSymbolValue = 0;
    Expected<section_iterator> TargetSecOrErr = Sym->getSection();
    if (TargetSecOrErr) {
      if (*TargetSecOrErr != Obj.section_end())
        TargetSectionIndex = (*TargetSecOrErr)->getIndex();
    } else {
      consumeError(TargetSecOrErr.takeError());
    }

    Expected<uint64_t> TargetValueOrErr = Sym->getValue();
    if (TargetValueOrErr) {
      TargetSymbolValue = *TargetValueOrErr;
    } else {
      consumeError(TargetValueOrErr.takeError());
    }

    Expected<int64_t> AddendOrErr = ELFRelocationRef(Rel).getAddend();
    if (!AddendOrErr) {
      lld::error(Path + ": " + toString(AddendOrErr.takeError()));
      continue;
    }
    int64_t Addend = *AddendOrErr;
    Chunk.Relocs.push_back(
        {Offset, Type, std::string(*NameOrErr),
         TargetSectionIndex ? std::optional<uint64_t>(ObjIndex) : std::nullopt,
         TargetSectionIndex, TargetSymbolValue, Addend});
  }
}

static SectionChunk *findChunk(InputState &State, uint64_t ObjIndex,
                               uint64_t SectionIndex) {
  for (SectionChunk &Chunk : State.Text) {
    if (Chunk.ObjectIndex == ObjIndex && Chunk.SectionIndex == SectionIndex)
      return &Chunk;
  }
  for (SectionChunk &Chunk : State.ProgramData) {
    if (Chunk.ObjectIndex == ObjIndex && Chunk.SectionIndex == SectionIndex)
      return &Chunk;
  }
  return nullptr;
}

static void readObject(ObjectFile &Obj, StringRef Path, InputState &State) {
  if (Obj.getArch() != Triple::abc) {
    lld::error(Path + ": expected ABC ELF object");
    return;
  }

  uint64_t ObjIndex = State.NextObjectIndex++;
  for (const SectionRef &Sec : Obj.sections()) {
    Expected<StringRef> NameOrErr = Sec.getName();
    if (!NameOrErr) {
      lld::error(Path + ": " + toString(NameOrErr.takeError()));
      continue;
    }

    StringRef Name = *NameOrErr;
    if (Name == ".bss") {
      State.Ram.push_back({std::string(Name), ObjIndex, Sec.getIndex(), 0,
                           Sec.getSize(), false});
      State.Symbols.push_back(
          {std::string(Name), ObjIndex, Sec.getIndex(), 0, false});
      continue;
    }
    if (Name == ".abc.saved") {
      State.Ram.push_back({std::string(Name), ObjIndex, Sec.getIndex(), 0,
                           Sec.getSize(), true});
      State.Symbols.push_back(
          {std::string(Name), ObjIndex, Sec.getIndex(), 0, false});
      continue;
    }
    bool IsText = Name == ".text" || Name.starts_with(".text.$globinit.");
    if (!IsText && Name != ".rodata")
      continue;

    Expected<StringRef> ContentsOrErr = Sec.getContents();
    if (!ContentsOrErr) {
      lld::error(Path + ": " + toString(ContentsOrErr.takeError()));
      continue;
    }

    SectionChunk Chunk = makeSection(ObjIndex, Sec, Name, *ContentsOrErr);
    if (IsText)
      State.Text.push_back(std::move(Chunk));
    else
      State.ProgramData.push_back(std::move(Chunk));
  }

  for (const SectionRef &Sec : Obj.sections()) {
    Expected<section_iterator> RelocatedSecOrErr =
        Sec.getRelocatedSection();
    if (!RelocatedSecOrErr) {
      consumeError(RelocatedSecOrErr.takeError());
      continue;
    }
    if (*RelocatedSecOrErr == Obj.section_end())
      continue;

    SectionChunk *Chunk =
        findChunk(State, ObjIndex, (*RelocatedSecOrErr)->getIndex());
    if (!Chunk)
      continue;
    readRelocations(Obj, ObjIndex, Sec, Path, *Chunk);
  }

  for (const SymbolRef &Sym : Obj.symbols()) {
    Expected<StringRef> NameOrErr = Sym.getName();
    if (!NameOrErr) {
      consumeError(NameOrErr.takeError());
      continue;
    }
    if (NameOrErr->empty())
      continue;

    Expected<uint32_t> FlagsOrErr = Sym.getFlags();
    if (!FlagsOrErr) {
      consumeError(FlagsOrErr.takeError());
      continue;
    }
    bool IsAbsolute = (*FlagsOrErr & SymbolRef::SF_Absolute) != 0;

    Expected<section_iterator> SecOrErr = Sym.getSection();
    if (!SecOrErr) {
      consumeError(SecOrErr.takeError());
      continue;
    }
    if (!IsAbsolute && *SecOrErr == Obj.section_end())
      continue;

    Expected<uint64_t> ValueOrErr = Sym.getValue();
    if (!ValueOrErr) {
      consumeError(ValueOrErr.takeError());
      continue;
    }

    State.Symbols.push_back({std::string(*NameOrErr), ObjIndex,
                             IsAbsolute ? std::nullopt
                                        : std::optional<uint64_t>((*SecOrErr)->getIndex()),
                             *ValueOrErr, IsAbsolute});
  }
}

static void readInput(StringRef Path, InputState &State) {
  Expected<OwningBinary<Binary>> BinaryOrErr = createBinary(Path);
  if (!BinaryOrErr) {
    lld::error(Path + ": " + toString(BinaryOrErr.takeError()));
    return;
  }

  Binary &Bin = *BinaryOrErr->getBinary();
  if (auto *ArchiveFile = dyn_cast<Archive>(&Bin)) {
    Error Err = Error::success();
    for (const Archive::Child &Child : ArchiveFile->children(Err)) {
      std::string MemberName = "<archive member>";
      if (Expected<StringRef> NameOrErr = Child.getName())
        MemberName = std::string(*NameOrErr);
      else
        consumeError(NameOrErr.takeError());

      Expected<std::unique_ptr<Binary>> ChildOrErr = Child.getAsBinary();
      if (!ChildOrErr) {
        lld::error(Path + "(" + MemberName + "): " +
                   toString(ChildOrErr.takeError()));
        continue;
      }

      if (auto *Obj = dyn_cast<ObjectFile>(&*ChildOrErr.get())) {
        std::string ChildPath = (Path + "(" + MemberName + ")").str();
        readObject(*Obj, ChildPath, State);
        continue;
      }

      lld::error(Path + "(" + MemberName + "): unsupported archive member");
    }
    if (Err)
      lld::error(Path + ": " + toString(std::move(Err)));
    return;
  }

  if (auto *Obj = dyn_cast<ObjectFile>(&Bin)) {
    readObject(*Obj, Path, State);
    return;
  }

  lld::error(Path + ": unsupported input file");
}

static std::optional<uint64_t> findTextSymbol(const InputState &State,
                                              StringRef Name) {
  for (const SymbolDef &Sym : State.Symbols) {
    if (Sym.Name != Name || !Sym.SectionIndex)
      continue;
    for (const SectionChunk &Chunk : State.Text) {
      if (Chunk.ObjectIndex == Sym.ObjectIndex &&
          Chunk.SectionIndex == *Sym.SectionIndex)
        return Chunk.OutputOffset + Sym.Value;
    }
  }
  return std::nullopt;
}

static std::optional<uint64_t> findProgramSymbol(const InputState &State,
                                                 StringRef Name) {
  for (const SymbolDef &Sym : State.Symbols) {
    if (Sym.Name != Name || !Sym.SectionIndex)
      continue;
    for (const SectionChunk &Chunk : State.Text) {
      if (Chunk.ObjectIndex == Sym.ObjectIndex &&
          Chunk.SectionIndex == *Sym.SectionIndex)
        return Chunk.OutputOffset + Sym.Value;
    }
    for (const SectionChunk &Chunk : State.ProgramData) {
      if (Chunk.ObjectIndex == Sym.ObjectIndex &&
          Chunk.SectionIndex == *Sym.SectionIndex)
        return Chunk.OutputOffset + Sym.Value;
    }
  }
  return std::nullopt;
}

static std::optional<uint64_t> findAbsoluteSymbol(const InputState &State,
                                                  StringRef Name) {
  for (const SymbolDef &Sym : State.Symbols)
    if (Sym.Name == Name && Sym.Absolute)
      return Sym.Value;
  return std::nullopt;
}

struct InitStubTarget {
  uint64_t ObjectIndex = 0;
  uint64_t SectionIndex = 0;
  uint64_t Value = 0;
};

static std::vector<InitStubTarget> collectInitStubTargets(const InputState &State) {
  std::vector<InitStubTarget> Targets;
  auto AddTarget = [&](uint64_t ObjectIndex, uint64_t SectionIndex,
                       uint64_t Value) {
    for (const InitStubTarget &Target : Targets) {
      if (Target.ObjectIndex == ObjectIndex &&
          Target.SectionIndex == SectionIndex && Target.Value == Value)
        return;
    }
    Targets.push_back({ObjectIndex, SectionIndex, Value});
  };

  for (const SectionChunk &Chunk : State.Text) {
    if (Chunk.IsInitStubSection)
      AddTarget(Chunk.ObjectIndex, Chunk.SectionIndex, 0);
  }

  for (const SymbolDef &Sym : State.Symbols) {
    if (Sym.Absolute || !Sym.SectionIndex)
      continue;
    if (!StringRef(Sym.Name).starts_with("$globinit."))
      continue;
    AddTarget(Sym.ObjectIndex, *Sym.SectionIndex, Sym.Value);
  }

  return Targets;
}

static std::optional<uint64_t> resolveInitStubAddress(const InputState &State,
                                                      const InitStubTarget &Target) {
  for (const SectionChunk &Chunk : State.Text) {
    if (Chunk.ObjectIndex == Target.ObjectIndex &&
        Chunk.SectionIndex == Target.SectionIndex)
      return Chunk.OutputOffset + Target.Value;
  }
  return std::nullopt;
}

static std::optional<uint64_t> findGlobalSymbol(const InputState &State,
                                                StringRef Name) {
  for (const SymbolDef &Sym : State.Symbols) {
    if (Sym.Name != Name || !Sym.SectionIndex)
      continue;
    for (const RamSection &Section : State.Ram) {
      if (Section.ObjectIndex == Sym.ObjectIndex &&
          Section.SectionIndex == *Sym.SectionIndex)
        return Section.OutputOffset + Sym.Value;
    }
  }
  return std::nullopt;
}

static std::optional<int64_t>
findRelocationTarget(const InputState &State, const SectionChunk::Reloc &Rel) {
  if (!Rel.SymbolName.empty()) {
    if (std::optional<uint64_t> Symbol =
            findProgramSymbol(State, Rel.SymbolName))
      return int64_t(*Symbol) + Rel.Addend;
  }

  if (!Rel.TargetObjectIndex || !Rel.TargetSectionIndex)
    return std::nullopt;

  auto FindChunk = [&](ArrayRef<SectionChunk> Chunks)
      -> std::optional<int64_t> {
    for (const SectionChunk &Chunk : Chunks) {
      if (Chunk.ObjectIndex == *Rel.TargetObjectIndex &&
          Chunk.SectionIndex == *Rel.TargetSectionIndex)
        return int64_t(Chunk.OutputOffset + Rel.TargetSymbolValue) +
               Rel.Addend;
    }
    return std::nullopt;
  };

  if (std::optional<int64_t> Target = FindChunk(State.Text))
    return Target;
  if (std::optional<int64_t> Target = FindChunk(State.ProgramData))
    return Target;
  for (const RamSection &Section : State.Ram) {
    if (Section.ObjectIndex == *Rel.TargetObjectIndex &&
        Section.SectionIndex == *Rel.TargetSectionIndex)
      return int64_t(Section.OutputOffset + Rel.TargetSymbolValue) +
             Rel.Addend;
  }
  return std::nullopt;
}

static std::optional<int64_t>
findGlobalRelocationTarget(const InputState &State,
                           const SectionChunk::Reloc &Rel) {
  if (!Rel.SymbolName.empty()) {
    if (std::optional<uint64_t> Symbol =
            findGlobalSymbol(State, Rel.SymbolName))
      return int64_t(*Symbol) + Rel.Addend;
  }

  if (!Rel.TargetObjectIndex || !Rel.TargetSectionIndex)
    return std::nullopt;

  for (const RamSection &Section : State.Ram) {
    if (Section.ObjectIndex == *Rel.TargetObjectIndex &&
        Section.SectionIndex == *Rel.TargetSectionIndex)
      return int64_t(Section.OutputOffset + Rel.TargetSymbolValue) +
             Rel.Addend;
  }
  return std::nullopt;
}

static uint64_t globalLimit(uint8_t Shades) {
  return Shades == 2 ? 1024 : 256;
}

static void layoutGlobals(InputState &State, const Config &C) {
  State.SavedBytes = 0;
  State.BssBytes = 0;

  uint64_t Offset = 0;
  for (RamSection &Section : State.Ram) {
    if (!Section.Saved)
      continue;
    Section.OutputOffset = Offset;
    Offset += Section.Size;
  }
  State.SavedBytes = Offset;

  for (RamSection &Section : State.Ram) {
    if (Section.Saved)
      continue;
    Section.OutputOffset = Offset;
    Offset += Section.Size;
  }
  State.BssBytes = Offset - State.SavedBytes;

  uint64_t Limit = globalLimit(C.Shades);
  uint64_t SaveBytes = C.SaveSize.value_or(State.SavedBytes);
  if (SaveBytes > Limit)
    lld::error("saved globals exceed active shade-mode RAM limit");
  if (Offset > Limit)
    lld::error("global RAM exceeds active shade-mode limit");
}

static bool fitsSigned(int64_t Value, unsigned Bits) {
  int64_t Min = -(int64_t(1) << (Bits - 1));
  int64_t Max = (int64_t(1) << (Bits - 1)) - 1;
  return Value >= Min && Value <= Max;
}

static bool fitsUnsigned(uint64_t Value, unsigned Bits) {
  return Bits == 64 || Value < (uint64_t(1) << Bits);
}

static void applyRelocations(std::vector<uint8_t> &Image,
                             const InputState &State,
                             ArrayRef<SectionChunk> Chunks) {
  for (const SectionChunk &Chunk : Chunks) {
    for (const SectionChunk::Reloc &Rel : Chunk.Relocs) {
      unsigned Width = relocWidth(Rel.Type);
      uint64_t PatchOffset = Chunk.OutputOffset + Rel.Offset;

      switch (Rel.Type) {
      case ELF::R_ABC_GLOBAL16_TAGGED:
      case ELF::R_ABC_GLOBAL8: {
        std::optional<int64_t> Target =
            findGlobalRelocationTarget(State, Rel);
        if (!Target) {
          lld::error("undefined global symbol: " + Rel.SymbolName);
          continue;
        }
        if (*Target < 0 || *Target >= int64_t(globalLimit(2))) {
          lld::error("global relocation out of range for symbol " +
                     Rel.SymbolName);
          continue;
        }

        uint64_t Value = uint64_t(*Target);
        if (Rel.Type == ELF::R_ABC_GLOBAL16_TAGGED) {
          writeLittle(Image, PatchOffset, 0x0200 + Value, Width);
          continue;
        }
        if (!fitsUnsigned(Value, 8)) {
          lld::error("short global relocation out of range for symbol " +
                     Rel.SymbolName);
          continue;
        }
        writeLittle(Image, PatchOffset, Value, Width);
        continue;
      }
      case ELF::R_ABC_BRANCH8:
      case ELF::R_ABC_BRANCH16: {
        std::optional<int64_t> Target = findRelocationTarget(State, Rel);
        if (!Target) {
          lld::error("undefined symbol: " + Rel.SymbolName);
          continue;
        }
        int64_t Value = *Target;
        Value -= int64_t(PatchOffset + Width);
        if (!fitsSigned(Value, Width * 8)) {
          lld::error("branch relocation out of range for symbol " +
                     Rel.SymbolName);
          continue;
        }
        writeLittle(Image, PatchOffset, uint64_t(Value), Width);
        continue;
      }
      case ELF::R_ABC_8:
      case ELF::R_ABC_16:
      case ELF::R_ABC_24:
      case ELF::R_ABC_32:
      case ELF::R_ABC_PROG24:
      case ELF::R_ABC_CALL24: {
        std::optional<int64_t> Target = findRelocationTarget(State, Rel);
        if (!Target) {
          lld::error("undefined symbol: " + Rel.SymbolName);
          continue;
        }
        int64_t Value = *Target;
        if (Value < 0 || !fitsUnsigned(uint64_t(Value), Width * 8)) {
          lld::error("relocation out of range for symbol " + Rel.SymbolName);
          continue;
        }
        writeLittle(Image, PatchOffset, uint64_t(Value), Width);
        continue;
      }
      default:
        lld::error("unsupported ABC relocation type " + Twine(Rel.Type).str());
        continue;
      }
    }
  }
}

static void appendStartup(std::vector<uint8_t> &Image, uint32_t GlobInit,
                          uint32_t Main, uint8_t MainReturnBytes) {
  Image[0x14] = ABC_OP_CALL;
  set24(Image, 0x15, GlobInit);
  if (MainReturnBytes) {
    Image[0x18] = ABC_OP_ALLOC;
    Image[0x19] = MainReturnBytes;
    Image[0x1a] = ABC_OP_CALL;
    set24(Image, 0x1b, Main);
    Image[0x1e] = ABC_OP_JMP1;
    Image[0x1f] = uint8_t(int8_t(0x18 - 0x20));
    return;
  }

  Image[0x18] = ABC_OP_CALL;
  set24(Image, 0x19, Main);
  Image[0x1c] = ABC_OP_JMP1;
  Image[0x1d] = uint8_t(int8_t(0x18 - 0x1e));
}

static void synthesizeGlobInit(std::vector<uint8_t> &Image, uint64_t Offset,
                               const InputState &State,
                               ArrayRef<InitStubTarget> Targets) {
  size_t Cursor = static_cast<size_t>(Offset);
  for (const InitStubTarget &Target : Targets) {
    std::optional<uint64_t> Address = resolveInitStubAddress(State, Target);
    if (!Address) {
      lld::error("unable to resolve synthesized $globinit stub target");
      return;
    }
    if (*Address > 0xFFFFFF) {
      lld::error("init stub is outside ABC 24-bit program space");
      return;
    }
    Image[Cursor++] = ABC_OP_CALL;
    set24(Image, Cursor, static_cast<uint32_t>(*Address));
    Cursor += 3;
  }
  Image[Cursor] = ABC_OP_RET;
}

static void appendTrailer(std::vector<uint8_t> &Image) {
  size_t PaddedSize = ((Image.size() + 6 + 255) / 256) * 256;
  Image.resize(PaddedSize - 6, 0);

  // The Arduboy startup code reads these two bytes from immediately before
  // ABC_END_SIGNATURE and uses them as the page containing the beginning of
  // the program data.  ABC binaries are placed at the end of FX flash, so
  // this is the page number counted from the beginning of the 16 MiB device.
  uint16_t DataPage = uint16_t(0x10000 - PaddedSize / 256);
  Image.push_back(uint8_t(DataPage));
  Image.push_back(uint8_t(DataPage >> 8));
  Image.insert(Image.end(), std::begin(ABC_END_SIGNATURE),
               std::end(ABC_END_SIGNATURE));
}

static std::vector<uint8_t> buildImage(const Config &C, InputState &State) {
  layoutGlobals(State, C);
  if (lld::errorCount())
    return {};

  std::vector<uint8_t> Image(256, 0);
  llvm::copy(ABC_SIGNATURE, Image.begin());
  Image[0x13] = C.Shades;

  for (SectionChunk &Chunk : State.ProgramData) {
    Chunk.OutputOffset = Image.size();
    Image.insert(Image.end(), Chunk.Contents.begin(), Chunk.Contents.end());
  }

  uint64_t CodeStart = Image.size();
  bool HasObjectGlobInit = findTextSymbol(State, "$globinit").has_value();
  std::vector<InitStubTarget> InitStubs;
  uint64_t SyntheticGlobInit = CodeStart;
  if (!HasObjectGlobInit) {
    InitStubs = collectInitStubTargets(State);
    size_t SyntheticGlobInitSize =
        InitStubs.empty() ? 1 : (InitStubs.size() * 4) + 1;
    Image.resize(Image.size() + SyntheticGlobInitSize, 0);
  }

  for (SectionChunk &Chunk : State.Text) {
    Chunk.OutputOffset = Image.size();
    Image.insert(Image.end(), Chunk.Contents.begin(), Chunk.Contents.end());
  }

  if (!HasObjectGlobInit) {
    if (InitStubs.empty()) {
      Image[static_cast<size_t>(SyntheticGlobInit)] = ABC_OP_RET;
    } else {
      synthesizeGlobInit(Image, SyntheticGlobInit, State, InitStubs);
      if (lld::errorCount())
        return {};
    }
  }

  applyRelocations(Image, State, State.ProgramData);
  applyRelocations(Image, State, State.Text);

  std::optional<uint64_t> Main = findTextSymbol(State, "main");
  if (!Main) {
    lld::error("undefined symbol: main");
    return {};
  }
  std::optional<uint64_t> MainReturnBytes =
      findAbsoluteSymbol(State, ABC_MAIN_RETURN_BYTES_SYMBOL);
  if (!MainReturnBytes) {
    lld::error("missing ABC main return metadata");
    return {};
  }
  if (*MainReturnBytes > UINT8_MAX) {
    lld::error("ABC main return metadata is out of range");
    return {};
  }
  std::optional<uint64_t> GlobInit = findTextSymbol(State, "$globinit");
  if (!GlobInit)
    GlobInit = SyntheticGlobInit;

  if (*Main > 0xFFFFFF || *GlobInit > 0xFFFFFF)
    lld::error("entry point is outside ABC 24-bit program space");

  uint64_t FileTableOffset = Image.size();
  uint64_t LineTableOffset = Image.size();
  if (FileTableOffset > 0xFFFFFF || LineTableOffset > 0xFFFFFF)
    lld::error("debug table is outside ABC 24-bit program space");

  uint64_t SaveBytes = C.SaveSize.value_or(State.SavedBytes);
  if (SaveBytes > UINT16_MAX)
    lld::error("saved globals exceed 65535 bytes");

  if (lld::errorCount())
    return {};

  set16(Image, 0x0a, SaveBytes);
  Image[0x0c] = 0;
  set24(Image, 0x0d, FileTableOffset);
  set24(Image, 0x10, LineTableOffset);
  appendStartup(Image, *GlobInit, *Main, uint8_t(*MainReturnBytes));
  appendTrailer(Image);
  return Image;
}

static void writeFile(StringRef Path, ArrayRef<uint8_t> Image) {
  Expected<std::unique_ptr<FileOutputBuffer>> BufferOrErr =
      FileOutputBuffer::create(Path, Image.size());
  if (!BufferOrErr) {
    lld::error(Path + ": " + toString(BufferOrErr.takeError()));
    return;
  }

  std::unique_ptr<FileOutputBuffer> Buffer = std::move(*BufferOrErr);
  llvm::copy(Image, Buffer->getBufferStart());
  if (Error E = Buffer->commit())
    lld::error(Path + ": " + toString(std::move(E)));
}

} // namespace

bool link(ArrayRef<const char *> Args, raw_ostream &StdoutOS,
          raw_ostream &StderrOS, bool ExitEarly, bool DisableOutput) {
  auto *Context = new CommonLinkerContext;
  Context->e.initialize(StdoutOS, StderrOS, ExitEarly, DisableOutput);
  Context->e.logName = sys::path::filename(Args[0]);

  std::optional<Config> C = parseArgs(Args, StdoutOS);
  if (!C)
    return lld::errorCount() == 0;

  InputState State;
  for (const std::string &Path : C->Inputs)
    readInput(Path, State);
  if (lld::errorCount())
    return false;

  std::vector<uint8_t> Image = buildImage(*C, State);
  if (lld::errorCount())
    return false;

  if (!DisableOutput)
    writeFile(C->Output, Image);
  return lld::errorCount() == 0;
}

} // namespace lld::abc
