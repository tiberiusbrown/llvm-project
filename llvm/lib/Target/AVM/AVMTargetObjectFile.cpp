//===-- AVMTargetObjectFile.cpp - AVM ELF section lowering ---------------===//

#include "AVMTargetObjectFile.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSymbolELF.h"

using namespace llvm;

static MCSection *prepareStructorSection(MCSection *Section) {
  auto *ELFSection = static_cast<MCSectionELF *>(Section);
  ELFSection->setFlags((ELFSection->getFlags() & ~ELF::SHF_WRITE) |
                       ELF::SHF_AVM_PROGSPACE);
  ELFSection->setEntrySize(3);
  return Section;
}

MCSection *AVMTargetObjectFile::getStaticCtorSection(
    unsigned Priority, const MCSymbol *KeySym) const {
  return prepareStructorSection(
      TargetLoweringObjectFileELF::getStaticCtorSection(Priority, KeySym));
}

MCSection *AVMTargetObjectFile::getStaticDtorSection(
    unsigned Priority, const MCSymbol *KeySym) const {
  return prepareStructorSection(
      TargetLoweringObjectFileELF::getStaticDtorSection(Priority, KeySym));
}

SectionKind AVMTargetObjectFile::getAVMSectionKind(const GlobalObject *GO,
                                                   SectionKind Kind) const {
  if (isa<Function>(GO))
    return SectionKind::getText();

  if (GO->getAddressSpace() == 0)
    return SectionKind::getData();

  if (GO->getAddressSpace() != 1) {
    getContext().reportError(SMLoc(), "unsupported AVM address space " +
                                          Twine(GO->getAddressSpace()) +
                                          " for object '" + GO->getName() +
                                          "'");
    return Kind;
  }

  const auto *GV = dyn_cast<GlobalVariable>(GO);
  if (!GV || !GV->isConstant())
    getContext().reportError(
        SMLoc(), "writable AVM address-space-one object is unsupported");

  return Kind;
}

// Clang clears GlobalVariable::isConstant for objects that need runtime
// initialization or contain writable mutable subobjects, so constness alone
// decides placement. Declarations, thread-local objects, and sections chosen by
// the user stay with their existing placement. Whole-program devirtualization
// splits vtables into address-space-zero blobs that carry type metadata, and
// dispatch reads them with program-space loads, so those blobs stay in data
// space.
bool AVMTargetObjectFile::isAutoFlashGlobal(const GlobalObject *GO) {
  const auto *GV = dyn_cast<GlobalVariable>(GO);
  return GV && GV->getAddressSpace() == 0 && GV->isConstant() &&
         GV->hasInitializer() && !GV->isThreadLocal() &&
         !GV->isExternallyInitialized() && !GV->hasSection() &&
         !GV->hasImplicitSection() && !GV->isDeclarationForLinker() &&
         !GV->hasAppendingLinkage() && !GV->hasMetadata(LLVMContext::MD_type);
}

// The base lowering names a read-only global .rodata with an optional symbol or
// mergeable suffix. Keep every attribute of that section, including the group,
// COMDAT state, unique ID, and merge entry size, and change only its namespace.
MCSection *AVMTargetObjectFile::moveToFlash(MCSection *Section) const {
  auto *ELFSection = static_cast<MCSectionELF *>(Section);
  StringRef Suffix = ELFSection->getName();
  if (!Suffix.consume_front(".rodata")) {
    getContext().reportError(SMLoc(), "AVM flash object lowered to section '" +
                                          ELFSection->getName() +
                                          "' outside .rodata");
    return Section;
  }
  // A mergeable section can be shared with an AS1 object of the same kind, and
  // that object's classification has already set a program-space bit here.
  // Classification of the flash copy supplies the data-space bit.
  unsigned Flags = ELFSection->getFlags() &
                   ~(ELF::SHF_AVM_PROGSPACE | ELF::SHF_AVM_DATASPACE);
  const MCSymbol *LinkedTo = ELFSection->getLinkedToSymbol();
  return getContext().getELFSection(
      (Twine(".flash") + Suffix).str(), ELFSection->getType(), Flags,
      ELFSection->getEntrySize(), ELFSection->getGroup(),
      ELFSection->isComdat(), ELFSection->getUniqueID(),
      LinkedTo ? cast<MCSymbolELF>(LinkedTo) : nullptr);
}

MCSection *AVMTargetObjectFile::classifySection(MCSection *Section,
                                                AVMSectionClass Class) const {
  auto *ELFSection = static_cast<MCSectionELF *>(Section);
  unsigned Flags = ELFSection->getFlags();
  if (!(Flags & ELF::SHF_ALLOC))
    return Section;

  const bool IsDataSpace = Class == AVMSectionClass::Data ||
                           Class == AVMSectionClass::Flash;
  const unsigned RequiredSpaceFlag =
      IsDataSpace ? ELF::SHF_AVM_DATASPACE : ELF::SHF_AVM_PROGSPACE;
  const unsigned OppositeSpaceFlag =
      IsDataSpace ? ELF::SHF_AVM_PROGSPACE : ELF::SHF_AVM_DATASPACE;

  if (Flags & OppositeSpaceFlag)
    getContext().reportError(
        SMLoc(), "AVM section '" + ELFSection->getName() +
                     "' cannot be shared by program-space and data-space "
                     "objects");

  Flags |= RequiredSpaceFlag;
  ELFSection->setFlags(Flags);

  if (Class == AVMSectionClass::ProgramText) {
    if (!(Flags & ELF::SHF_EXECINSTR))
      getContext().reportError(SMLoc(), "AVM program-space text section '" +
                                            ELFSection->getName() +
                                            "' is not executable");
    if (Flags & ELF::SHF_WRITE)
      getContext().reportError(SMLoc(), "AVM program-space text section '" +
                                            ELFSection->getName() +
                                            "' is writable");
  } else if (Class == AVMSectionClass::ProgramData) {
    if (Flags & ELF::SHF_EXECINSTR)
      getContext().reportError(SMLoc(), "AVM program-space data section '" +
                                            ELFSection->getName() +
                                            "' is executable");
    if (Flags & ELF::SHF_WRITE)
      getContext().reportError(SMLoc(), "AVM program-space data section '" +
                                            ELFSection->getName() +
                                            "' is writable");
  } else if (Class == AVMSectionClass::Flash) {
    if (Flags & ELF::SHF_WRITE)
      getContext().reportError(SMLoc(), "AVM flash section '" +
                                            ELFSection->getName() +
                                            "' is writable");
    if (Flags & ELF::SHF_EXECINSTR)
      getContext().reportError(SMLoc(), "AVM flash section '" +
                                            ELFSection->getName() +
                                            "' is executable");
    if (ELFSection->getType() != ELF::SHT_PROGBITS)
      getContext().reportError(SMLoc(), "AVM flash section '" +
                                            ELFSection->getName() +
                                            "' is not SHT_PROGBITS");
  } else {
    if (!(Flags & ELF::SHF_WRITE))
      getContext().reportError(SMLoc(), "AVM data-space section '" +
                                            ELFSection->getName() +
                                            "' is not writable");
    if (Flags & ELF::SHF_EXECINSTR)
      getContext().reportError(SMLoc(), "AVM data-space section '" +
                                            ELFSection->getName() +
                                            "' is executable");
    if (ELFSection->getType() == ELF::SHT_NOBITS)
      getContext().reportError(SMLoc(), "AVM data-space section '" +
                                            ELFSection->getName() +
                                            "' uses unsupported .bss storage");
    else if (ELFSection->getType() != ELF::SHT_PROGBITS)
      getContext().reportError(SMLoc(), "AVM data-space section '" +
                                            ELFSection->getName() +
                                            "' is not SHT_PROGBITS");
  }

  return Section;
}

MCSection *AVMTargetObjectFile::getExplicitSectionGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {
  Kind = getAVMSectionKind(GO, Kind);
  MCSection *Section =
      TargetLoweringObjectFileELF::getExplicitSectionGlobal(GO, Kind, TM);
  if (isa<Function>(GO))
    return classifySection(Section, AVMSectionClass::ProgramText);
  return classifySection(Section, GO->getAddressSpace() == 0
                                      ? AVMSectionClass::Data
                                      : AVMSectionClass::ProgramData);
}

MCSection *AVMTargetObjectFile::SelectSectionForGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {
  if (isAutoFlashGlobal(GO)) {
    // Mergeable literals keep their merge kind. Any other constant becomes
    // ordinary read-only data, so zero-initialized and relocated objects still
    // have explicit PROGBITS contents.
    if (!Kind.isMergeableCString() && !Kind.isMergeableConst())
      Kind = SectionKind::getReadOnly();
    return classifySection(
        moveToFlash(TargetLoweringObjectFileELF::SelectSectionForGlobal(
            GO, Kind, TM)),
        AVMSectionClass::Flash);
  }
  Kind = getAVMSectionKind(GO, Kind);
  MCSection *Section =
      TargetLoweringObjectFileELF::SelectSectionForGlobal(GO, Kind, TM);
  if (isa<Function>(GO))
    return classifySection(Section, AVMSectionClass::ProgramText);
  return classifySection(Section, GO->getAddressSpace() == 0
                                      ? AVMSectionClass::Data
                                      : AVMSectionClass::ProgramData);
}

MCSection *AVMTargetObjectFile::getUniqueSectionForFunction(
    const Function &F, const TargetMachine &TM) const {
  return classifySection(
      TargetLoweringObjectFileELF::getUniqueSectionForFunction(F, TM),
      AVMSectionClass::ProgramText);
}

MCSection *AVMTargetObjectFile::getSectionForMachineBasicBlock(
    const Function &F, const MachineBasicBlock &MBB,
    const TargetMachine &TM) const {
  return classifySection(
      TargetLoweringObjectFileELF::getSectionForMachineBasicBlock(F, MBB, TM),
      AVMSectionClass::ProgramText);
}

MCSection *AVMTargetObjectFile::getSectionForConstant(const DataLayout &DL,
                                                      SectionKind Kind,
                                                      const Constant *C,
                                                      Align &Alignment) const {
  return classifySection(TargetLoweringObjectFileELF::getSectionForConstant(
                             DL, Kind, C, Alignment),
                         AVMSectionClass::ProgramData);
}

MCSection *AVMTargetObjectFile::getSectionForConstant(
    const DataLayout &DL, SectionKind Kind, const Constant *C, Align &Alignment,
    StringRef SectionSuffix) const {
  return classifySection(TargetLoweringObjectFileELF::getSectionForConstant(
                             DL, Kind, C, Alignment, SectionSuffix),
                         AVMSectionClass::ProgramData);
}

MCSection *
AVMTargetObjectFile::getSectionForJumpTable(const Function &F,
                                            const TargetMachine &TM) const {
  return classifySection(
      TargetLoweringObjectFileELF::getSectionForJumpTable(F, TM),
      AVMSectionClass::ProgramData);
}

MCSection *AVMTargetObjectFile::getSectionForJumpTable(
    const Function &F, const TargetMachine &TM,
    const MachineJumpTableEntry *JTE) const {
  return classifySection(
      TargetLoweringObjectFileELF::getSectionForJumpTable(F, TM, JTE),
      AVMSectionClass::ProgramData);
}

MCSection *
AVMTargetObjectFile::getSectionForLSDA(const Function &F, const MCSymbol &FnSym,
                                       const TargetMachine &TM) const {
  return classifySection(
      TargetLoweringObjectFileELF::getSectionForLSDA(F, FnSym, TM),
      AVMSectionClass::ProgramData);
}

MCSection *AVMTargetObjectFile::getExceptionSitesSection(
    const Function &F, const MCSymbol &FnSym, const TargetMachine &TM) const {
  // Grouped and linked with the function text, so discarded functions drop
  // their site records with them.
  const auto &Text = cast<MCSectionELF>(FnSym.getSection());
  unsigned Flags = ELF::SHF_LINK_ORDER;
  StringRef Group;
  if (const MCSymbol *G = Text.getGroup()) {
    Group = G->getName();
    Flags |= ELF::SHF_GROUP;
  }
  return getContext().getELFSection(
      ".avm.eh_sites", ELF::SHT_PROGBITS, Flags, 0, Group, true,
      Text.getUniqueID(), cast<MCSymbolELF>(Text.getBeginSymbol()));
}
