//===-- llvm-avm-image.cpp - AVM flat image packer -----------------------===//

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/CRC.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/FileOutputBuffer.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MathExtras.h"
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
    cl::desc("Append a 4 KiB erased save area to the output image"));

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

static Error packageELF(StringRef InputName) {
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
  if (H.e_machine != 0x4156 || H.e_flags != 1)
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

  Expected<uint32_t> FileSize = finalFileSize(PayloadEnd);
  if (!FileSize)
    return FileSize.takeError();
  if (Development && *FileSize > MaxFlashSize - DevelopmentSaveSize)
    return bad("AVM development image leaves no room for a 4 KiB save area");
  SmallVector<uint8_t, 0> Image(*FileSize, 0xff);
  std::fill(Image.begin(), Image.begin() + HeaderSize, 0);
  Image[0] = 0x41; Image[1] = 0x56; Image[2] = 0x4d; Image[3] = 0x01;
  Image[4] = 1;
  write24(Image, 5, H.e_entry);
  write16le(Image.data() + 8, DataSize);
  write16le(Image.data() + 10, SaveSize);
  llvm::copy(*SavedBytes, Image.begin() + HeaderSize);
  llvm::copy(*DataBytes, Image.begin() + HeaderSize + SaveSize);
  for (const PayloadSection &S : Program)
    llvm::copy(S.Contents, Image.begin() + S.Address);
  size_t Tail = Image.size() - 8;
  Image[Tail] = 0x41; Image[Tail + 1] = 0x56;
  Image[Tail + 2] = 0x54; Image[Tail + 3] = 0x01;
  write16le(Image.data() + Tail + 4, *FileSize / PageSize);
  write16le(Image.data() + Tail + 6, 0);
  write32le(Image.data() + 0xfc, crc32(ArrayRef<uint8_t>(Image).take_front(0xfc)));

  // Keep the AVM tail and its page count tied to the executable image. The
  // optional save sector is raw erased flash outside the flat image.
  if (Development)
    Image.resize(Image.size() + DevelopmentSaveSize, 0xff);

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
  if (Error E = packageELF(Input)) {
    WithColor::error(errs(), "llvm-avm-image") << Input << ": "
                                                 << toString(std::move(E))
                                                 << '\n';
    return 1;
  }
  return 0;
}
