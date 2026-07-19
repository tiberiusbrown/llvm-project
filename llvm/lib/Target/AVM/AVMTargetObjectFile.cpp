//===-- AVMTargetObjectFile.cpp - AVM ELF section lowering ---------------===//

#include "AVMTargetObjectFile.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"

using namespace llvm;

void AVMTargetObjectFile::Initialize(MCContext &Ctx, const TargetMachine &TM) {
  TargetLoweringObjectFileELF::Initialize(Ctx, TM);
  DataSection = Ctx.getELFSection(".data", ELF::SHT_PROGBITS,
                                  ELF::SHF_ALLOC | ELF::SHF_WRITE |
                                      ELF::SHF_AVM_DATASPACE);
  ProgramDataSection = Ctx.getELFSection(
      ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC | ELF::SHF_AVM_PROGSPACE);
  ProgramTextSection = Ctx.getELFSection(".text", ELF::SHT_PROGBITS,
                                         ELF::SHF_ALLOC | ELF::SHF_EXECINSTR |
                                             ELF::SHF_AVM_PROGSPACE);
}

MCSection *AVMTargetObjectFile::getExplicitSectionGlobal(
    const GlobalObject *GO, SectionKind, const TargetMachine &) const {
  StringRef Name = GO->getSection();
  unsigned Flags = ELF::SHF_ALLOC;
  if (GO->getAddressSpace() == 0) {
    Flags |= ELF::SHF_WRITE | ELF::SHF_AVM_DATASPACE;
  } else if (isa<Function>(GO)) {
    Flags |= ELF::SHF_EXECINSTR | ELF::SHF_AVM_PROGSPACE;
  } else {
    Flags |= ELF::SHF_AVM_PROGSPACE;
  }
  return getContext().getELFSection(Name, ELF::SHT_PROGBITS, Flags);
}

MCSection *
AVMTargetObjectFile::SelectSectionForGlobal(const GlobalObject *GO, SectionKind,
                                            const TargetMachine &) const {
  if (GO->getAddressSpace() == 0)
    return DataSection;
  if (isa<Function>(GO))
    return ProgramTextSection;
  const auto *GV = dyn_cast<GlobalVariable>(GO);
  if (!GV || !GV->isConstant())
    getContext().reportError(
        SMLoc(), "writable AVM address-space-one object is unsupported");
  return ProgramDataSection;
}
