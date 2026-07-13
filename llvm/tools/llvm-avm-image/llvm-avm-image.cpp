//===-- llvm-avm-image.cpp - AVM early image utility ---------------------===//

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/CRC.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/FileOutputBuffer.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/WithColor.h"
#include <algorithm>
#include <cstring>
#include <optional>

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;

static cl::opt<std::string> Action(cl::Positional, cl::Required,
                                  cl::desc("<create|validate|inspect>"));
static cl::opt<std::string> Input(cl::Positional, cl::Required,
                                 cl::desc("<input>"));
static cl::opt<std::string> Output("o", cl::desc("Output image path"));
static cl::opt<unsigned> SaveSize("save-size", cl::init(0),
                                  cl::desc("Persistent save bytes (0..1024)"));
static cl::opt<unsigned> RuntimeVersion(
    "runtime-version", cl::init(1),
    cl::desc("Required AVM runtime compatibility version"));

static Error bad(const Twine &Message) {
  return createStringError(inconvertibleErrorCode(), Message);
}

static uint32_t read24(ArrayRef<uint8_t> Bytes, size_t Offset) {
  return Bytes[Offset] | uint32_t(Bytes[Offset + 1]) << 8 |
         uint32_t(Bytes[Offset + 2]) << 16;
}

static void write24(MutableArrayRef<uint8_t> Bytes, size_t Offset,
                    uint32_t Value) {
  Bytes[Offset] = Value;
  Bytes[Offset + 1] = Value >> 8;
  Bytes[Offset + 2] = Value >> 16;
}

static bool hasMagic(ArrayRef<uint8_t> Bytes, ArrayRef<uint8_t> Magic) {
  return Bytes.size() >= Magic.size() &&
         std::equal(Magic.begin(), Magic.end(), Bytes.begin());
}

static Error validateImage(ArrayRef<uint8_t> Bytes, bool Print) {
  static const uint8_t HeaderMagic[] = {0x41, 0x56, 0x4d, 0x01};
  static const uint8_t TailMagic[] = {0x41, 0x56, 0x54, 0x01};

  if (Bytes.size() < 512 || Bytes.size() % 256 != 0)
    return bad("image size must be at least 512 bytes and a multiple of 256");
  if (!hasMagic(Bytes, HeaderMagic))
    return bad("invalid AVM header magic");
  if (Bytes[4] == 0)
    return bad("runtimeVersion must be nonzero");

  uint32_t Entry = read24(Bytes, 5);
  uint16_t DataSize = read16le(Bytes.data() + 8);
  uint16_t StaticSize = read16le(Bytes.data() + 10);
  uint16_t PersistentSize = read16le(Bytes.data() + 12);
  if (DataSize > StaticSize || StaticSize > 1024)
    return bad("invalid dataSize/staticSize relationship");
  if (PersistentSize > 1024)
    return bad("saveSize exceeds 1024 bytes");
  if (llvm::any_of(Bytes.slice(14, 238),
                   [](uint8_t Byte) { return Byte != 0; }))
    return bad("reserved header bytes are not zero");

  uint32_t StoredCRC = read32le(Bytes.data() + 252);
  uint32_t ComputedCRC = crc32(Bytes.take_front(252));
  if (StoredCRC != ComputedCRC)
    return bad("header CRC-32 mismatch");

  ArrayRef<uint8_t> Tail = Bytes.take_back(8);
  if (!hasMagic(Tail, TailMagic))
    return bad("invalid AVM tail magic");
  uint16_t PageCount = read16le(Tail.data() + 4);
  if (PageCount == 0 || PageCount != Bytes.size() / 256)
    return bad("tail imagePageCount does not match file size");
  if (read16le(Tail.data() + 6) != 0)
    return bad("tail reserved bytes are not zero");

  uint32_t ProgramStart = alignTo(0x100u + DataSize, 0x100u);
  uint32_t TailOffset = Bytes.size() - 8;
  if ((Entry & 1) || Entry < ProgramStart || Entry >= TailOffset)
    return bad("entry point is outside the program payload or is not even");
  if (ProgramStart > TailOffset)
    return bad("programStart lies beyond the image payload");
  for (uint32_t I = 0x100u + DataSize; I < ProgramStart; ++I)
    if (Bytes[I] != 0xff)
      return bad("non-0xFF byte in data-to-program alignment padding");

  if (Print) {
    outs() << "AVM image\n"
           << "  fileSize: " << Bytes.size() << "\n"
           << "  imagePageCount: " << PageCount << "\n"
           << "  runtimeVersion: " << unsigned(Bytes[4]) << "\n"
           << "  entryPoint: 0x" << format_hex_no_prefix(Entry, 6) << "\n"
           << "  dataSize: " << DataSize << "\n"
           << "  staticSize: " << StaticSize << "\n"
           << "  saveSize: " << PersistentSize << "\n"
           << "  programStart: 0x"
           << format_hex_no_prefix(ProgramStart, 6) << "\n"
           << "  headerCrc32: 0x"
           << format_hex_no_prefix(StoredCRC, 8) << " (valid)\n";
  }
  return Error::success();
}

struct InputSection {
  SmallVector<uint8_t> Data;
  uint64_t Size = 0;
  bool Found = false;
};

static Expected<InputSection> findSection(const ObjectFile &Obj,
                                          StringRef Wanted) {
  InputSection Result;
  for (SectionRef Section : Obj.sections()) {
    Expected<StringRef> Name = Section.getName();
    if (!Name)
      return Name.takeError();
    if (*Name != Wanted)
      continue;
    if (Result.Found)
      return bad(Twine("multiple ") + Wanted + " sections are unsupported");
    Result.Found = true;
    Result.Size = Section.getSize();
    if (!Section.isBSS()) {
      Expected<StringRef> Contents = Section.getContents();
      if (!Contents)
        return Contents.takeError();
      Result.Data.append(Contents->bytes_begin(), Contents->bytes_end());
    }
  }
  return Result;
}

struct SectionLayout {
  StringRef Name;
  uint64_t Address;
  uint64_t Size;
};

static Expected<uint64_t>
getSymbolAddress(const ObjectFile &Obj, const SymbolRef &Symbol,
                 ArrayRef<SectionLayout> Layout) {
  Expected<section_iterator> SI = Symbol.getSection();
  if (!SI)
    return SI.takeError();
  if (*SI == Obj.section_end())
    return bad("relocation refers to an undefined or absolute symbol");
  Expected<StringRef> Name = (**SI).getName();
  if (!Name)
    return Name.takeError();
  Expected<uint64_t> Offset = Symbol.getAddress();
  if (!Offset)
    return Offset.takeError();
  for (const SectionLayout &Section : Layout)
    if (Section.Name == *Name) {
      if (*Offset > Section.Size)
        return bad(Twine("symbol lies outside section '") + *Name + "'");
      return Section.Address + *Offset;
    }
  return bad(Twine("relocation refers to unsupported section '") + *Name +
             "'");
}

static Error applyRelocations(const ObjectFile &Obj, StringRef SectionName,
                              MutableArrayRef<uint8_t> Data,
                              ArrayRef<SectionLayout> Layout) {
  for (SectionRef Section : Obj.sections()) {
    if (Section.relocation_begin() == Section.relocation_end())
      continue;
    Expected<section_iterator> Relocated = Section.getRelocatedSection();
    if (!Relocated)
      return Relocated.takeError();
    if (*Relocated == Obj.section_end())
      return bad("relocation section has no relocated section");
    Expected<StringRef> Name = (**Relocated).getName();
    if (!Name)
      return Name.takeError();
    if (*Name != SectionName)
      continue;

    uint64_t SectionAddress = 0;
    for (const SectionLayout &Entry : Layout)
      if (Entry.Name == SectionName)
        SectionAddress = Entry.Address;

    for (RelocationRef Reloc : Section.relocations()) {
      uint64_t Type = Reloc.getType();
      uint64_t Offset = Reloc.getOffset();
      unsigned Width = Type == ELF::R_AVM_PCREL8 ||
                               Type == ELF::R_AVM_PROG_HI8
                           ? 1
                           : Type == ELF::R_AVM_DATA16 ||
                                     Type == ELF::R_AVM_PROG_LO16 ||
                                     Type == ELF::R_AVM_BANK16
                                 ? 2
                                 : 3;
      if (Type == ELF::R_AVM_NONE || Type == ELF::R_AVM_RELAX)
        continue;
      if (Offset > Data.size() || Width > Data.size() - Offset)
        return bad("relocation field extends beyond its input section");

      symbol_iterator SymI = Reloc.getSymbol();
      if (SymI == Obj.symbol_end())
        return bad("relocation has no symbol");
      Expected<uint64_t> SymbolAddress =
          getSymbolAddress(Obj, *SymI, Layout);
      if (!SymbolAddress)
        return SymbolAddress.takeError();
      Expected<int64_t> Addend = ELFRelocationRef(Reloc).getAddend();
      if (!Addend)
        return Addend.takeError();
      int64_t SignedValue = static_cast<int64_t>(*SymbolAddress) + *Addend;
      if (Type == ELF::R_AVM_PCREL8)
        SignedValue -= static_cast<int64_t>(SectionAddress + Offset);
      if (Type != ELF::R_AVM_PCREL8 && SignedValue < 0)
        return bad("AVM relocation result is negative");
      uint64_t Value = static_cast<uint64_t>(SignedValue);
      uint8_t *Loc = Data.data() + Offset;

      switch (Type) {
      case ELF::R_AVM_DATA16:
        if (!isUInt<16>(Value))
          return bad("AVM data-space relocation is out of 16-bit range");
        write16le(Loc, Value);
        break;
      case ELF::R_AVM_PROG24:
        if (!isUInt<24>(Value))
          return bad("AVM program-space relocation is out of 24-bit range");
        Loc[0] = Value;
        Loc[1] = Value >> 8;
        Loc[2] = Value >> 16;
        break;
      case ELF::R_AVM_PROG_LO16:
        if (!isUInt<24>(Value))
          return bad("AVM program-space relocation is out of 24-bit range");
        write16le(Loc, Value);
        break;
      case ELF::R_AVM_PROG_HI8:
        if (!isUInt<24>(Value))
          return bad("AVM program-space relocation is out of 24-bit range");
        Loc[0] = Value >> 16;
        break;
      case ELF::R_AVM_BANK16:
        if (!isUInt<16>(Value))
          return bad("AVM same-bank target is out of 16-bit range");
        write16le(Loc, Value);
        break;
      case ELF::R_AVM_FAR24: {
        if (!isUInt<24>(Value))
          return bad("AVM far target is out of 24-bit range");
        if (Value & 1)
          return bad("AVM far target must be two-byte aligned");
        uint8_t Link = Loc[0] & 1;
        Loc[0] = static_cast<uint8_t>(Value & 0xfe) | Link;
        Loc[1] = Value >> 8;
        Loc[2] = Value >> 16;
        break;
      }
      case ELF::R_AVM_PCREL8:
        if (!isInt<8>(SignedValue))
          return bad("AVM relative displacement is out of signed 8-bit range");
        Loc[0] = static_cast<uint8_t>(SignedValue);
        break;
      default:
        return bad(Twine("unsupported AVM relocation type ") + Twine(Type));
      }
    }
  }
  return Error::success();
}

static Error checkObjectRestrictions(const ObjectFile &Obj) {
  for (SectionRef Section : Obj.sections()) {
    Expected<StringRef> Name = Section.getName();
    if (!Name)
      return Name.takeError();
    if (*Name == ".text" || *Name == ".rodata" || *Name == ".data" ||
        *Name == ".bss")
      continue;
    // Ignore normal ELF bookkeeping and debugging sections. Reject other
    // allocatable-looking contents rather than silently dropping payload.
    if (Name->empty() || Name->starts_with(".rel") ||
        Name->starts_with(".symtab") || Name->starts_with(".strtab") ||
        Name->starts_with(".shstrtab") || Name->starts_with(".debug") ||
        *Name == ".comment" || *Name == ".note.GNU-stack")
      continue;
    if (Section.getSize() != 0)
      return bad(Twine("unsupported input section '") + *Name + "'");
  }
  return Error::success();
}

static Error createImage() {
  if (Output.empty())
    return bad("create requires -o <output>");
  if (SaveSize > 1024)
    return bad("--save-size must be in range 0..1024");
  if (RuntimeVersion == 0 || RuntimeVersion > 255)
    return bad("--runtime-version must be in range 1..255");

  Expected<OwningBinary<ObjectFile>> Binary =
      ObjectFile::createObjectFile(Input);
  if (!Binary)
    return Binary.takeError();
  ObjectFile &Obj = *Binary->getBinary();
  if (!Obj.isELF() || Obj.getArch() != Triple::avm)
    return bad("input must be one AVM ELF relocatable object");

  auto *ELFObj = dyn_cast<ELF32LEObjectFile>(&Obj);
  if (!ELFObj)
    return bad("input must be little-endian ELF32");
  if (ELFObj->getELFFile().getHeader().e_type != ELF::ET_REL)
    return bad("early image mode requires an ET_REL object");
  if (Error E = checkObjectRestrictions(Obj))
    return E;

  Expected<InputSection> Text = findSection(Obj, ".text");
  if (!Text)
    return Text.takeError();
  Expected<InputSection> Rodata = findSection(Obj, ".rodata");
  if (!Rodata)
    return Rodata.takeError();
  Expected<InputSection> Data = findSection(Obj, ".data");
  if (!Data)
    return Data.takeError();
  Expected<InputSection> Bss = findSection(Obj, ".bss");
  if (!Bss)
    return Bss.takeError();

  if (!Text->Found || Text->Data.empty())
    return bad("exactly one nonempty .text section is required");
  if (Data->Size + Bss->Size > 1024)
    return bad(".data plus .bss exceeds 1024 bytes");

  std::optional<uint64_t> StartOffset;
  for (SymbolRef Symbol : Obj.symbols()) {
    Expected<StringRef> Name = Symbol.getName();
    if (!Name)
      return Name.takeError();
    if (*Name != "_start")
      continue;
    if (StartOffset)
      return bad("multiple _start symbols are unsupported");
    Expected<section_iterator> SI = Symbol.getSection();
    if (!SI)
      return SI.takeError();
    if (*SI == Obj.section_end())
      return bad("_start must be defined in .text");
    Expected<StringRef> SectionName = (**SI).getName();
    if (!SectionName)
      return SectionName.takeError();
    if (*SectionName != ".text")
      return bad("_start must be defined in .text");
    Expected<uint64_t> Address = Symbol.getAddress();
    if (!Address)
      return Address.takeError();
    StartOffset = *Address;
  }
  if (!StartOffset)
    return bad("required symbol _start is not defined");

  uint64_t ProgramStart = alignTo(0x100ull + Data->Size, 0x100ull);
  uint64_t Entry = ProgramStart + *StartOffset;
  if ((Entry & 1) || *StartOffset >= Text->Size)
    return bad("_start must be within .text and even-aligned");

  uint64_t PayloadEnd = ProgramStart + Text->Size + Rodata->Size;
  if (PayloadEnd > 0x10000)
    return bad("early image mode supports bank 0 only");
  uint64_t FileSize = alignTo(PayloadEnd + 8, 0x100ull);
  if (FileSize / 256 > 0xffff)
    return bad("image exceeds tail page-count range");

  const SectionLayout Layout[] = {
      {".data", 0x100, Data->Size},
      {".bss", 0x100 + Data->Size, Bss->Size},
      {".text", ProgramStart, Text->Size},
      {".rodata", ProgramStart + Text->Size, Rodata->Size},
  };
  if (Error E = applyRelocations(Obj, ".data", Data->Data, Layout))
    return E;
  if (Error E = applyRelocations(Obj, ".text", Text->Data, Layout))
    return E;
  if (Error E = applyRelocations(Obj, ".rodata", Rodata->Data, Layout))
    return E;

  SmallVector<uint8_t> Image(FileSize, 0xff);
  std::fill(Image.begin(), Image.begin() + 256, 0);
  Image[0] = 0x41;
  Image[1] = 0x56;
  Image[2] = 0x4d;
  Image[3] = 0x01;
  Image[4] = RuntimeVersion;
  write24(Image, 5, Entry);
  write16le(Image.data() + 8, Data->Size);
  write16le(Image.data() + 10, Data->Size + Bss->Size);
  write16le(Image.data() + 12, SaveSize);

  llvm::copy(Data->Data, Image.begin() + 0x100);
  llvm::copy(Text->Data, Image.begin() + ProgramStart);
  llvm::copy(Rodata->Data, Image.begin() + ProgramStart + Text->Size);
  write32le(Image.data() + 252,
            crc32(ArrayRef<uint8_t>(Image).take_front(252)));

  size_t TailOffset = Image.size() - 8;
  Image[TailOffset + 0] = 0x41;
  Image[TailOffset + 1] = 0x56;
  Image[TailOffset + 2] = 0x54;
  Image[TailOffset + 3] = 0x01;
  write16le(Image.data() + TailOffset + 4, Image.size() / 256);
  write16le(Image.data() + TailOffset + 6, 0);

  if (Error E = validateImage(Image, false))
    return E;

  Expected<std::unique_ptr<FileOutputBuffer>> Buffer =
      FileOutputBuffer::create(Output, Image.size());
  if (!Buffer)
    return Buffer.takeError();
  std::copy(Image.begin(), Image.end(), (*Buffer)->getBufferStart());
  return (*Buffer)->commit();
}

static Error readAndValidate(bool Print) {
  ErrorOr<std::unique_ptr<MemoryBuffer>> Buffer = MemoryBuffer::getFile(Input);
  if (!Buffer)
    return errorCodeToError(Buffer.getError());
  ArrayRef<uint8_t> Bytes(
      reinterpret_cast<const uint8_t *>((*Buffer)->getBufferStart()),
      (*Buffer)->getBufferSize());
  return validateImage(Bytes, Print);
}

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv, "AVM image utility\n");

  Error E = Action == "create"     ? createImage()
            : Action == "validate" ? readAndValidate(false)
            : Action == "inspect"  ? readAndValidate(true)
                                     : bad("unknown action; expected create, validate, or inspect");
  if (E) {
    WithColor::error(errs(), "llvm-avm-image") << toString(std::move(E))
                                                 << '\n';
    return 1;
  }
  if (Action == "validate")
    outs() << Input << ": valid AVM image\n";
  return 0;
}
