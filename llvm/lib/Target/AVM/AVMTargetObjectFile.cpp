//===-- AVMTargetObjectFile.cpp - AVM ELF section lowering ---------------===//

#include "AVMTargetObjectFile.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"

using namespace llvm;

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

MCSection *AVMTargetObjectFile::classifySection(MCSection *Section,
                                                AVMSectionClass Class) const {
  auto *ELFSection = static_cast<MCSectionELF *>(Section);
  unsigned Flags = ELFSection->getFlags();
  if (!(Flags & ELF::SHF_ALLOC))
    return Section;

  const bool IsDataSpace = Class == AVMSectionClass::Data;
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
