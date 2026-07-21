//===-- AVMTargetObjectFile.h - AVM ELF section lowering ------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMTARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_AVM_AVMTARGETOBJECTFILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {
class AVMTargetObjectFile final : public TargetLoweringObjectFileELF {
public:
  unsigned getTextSectionAlignment() const override { return 1; }

  MCSection *getExplicitSectionGlobal(const GlobalObject *GO, SectionKind Kind,
                                      const TargetMachine &TM) const override;
  MCSection *SelectSectionForGlobal(const GlobalObject *GO, SectionKind Kind,
                                    const TargetMachine &TM) const override;

  MCSection *
  getUniqueSectionForFunction(const Function &F,
                              const TargetMachine &TM) const override;

  MCSection *
  getSectionForMachineBasicBlock(const Function &F,
                                 const MachineBasicBlock &MBB,
                                 const TargetMachine &TM) const override;

  MCSection *getSectionForConstant(const DataLayout &DL, SectionKind Kind,
                                   const Constant *C,
                                   Align &Alignment) const override;

  MCSection *getSectionForConstant(const DataLayout &DL, SectionKind Kind,
                                   const Constant *C, Align &Alignment,
                                   StringRef SectionSuffix) const override;

  MCSection *getSectionForJumpTable(const Function &F,
                                    const TargetMachine &TM) const override;

  MCSection *
  getSectionForJumpTable(const Function &F, const TargetMachine &TM,
                         const MachineJumpTableEntry *JTE) const override;

  MCSection *getSectionForLSDA(const Function &F, const MCSymbol &FnSym,
                               const TargetMachine &TM) const override;

private:
  enum class AVMSectionClass { Data, ProgramData, ProgramText };

  SectionKind getAVMSectionKind(const GlobalObject *GO, SectionKind Kind) const;
  MCSection *classifySection(MCSection *Section, AVMSectionClass Class) const;
};
} // namespace llvm

#endif
