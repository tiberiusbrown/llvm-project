//===-- AVMTargetObjectFile.h - AVM ELF section lowering ------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMTARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_AVM_AVMTARGETOBJECTFILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {
class AVMTargetObjectFile final : public TargetLoweringObjectFileELF {
public:
  unsigned getTextSectionAlignment() const override { return 1; }

  void initialize(MCContext &Ctx, const TargetMachine &TM,
                  const Module &M) override;

  MCSection *getStaticCtorSection(unsigned Priority,
                                  const MCSymbol *KeySym) const override;
  MCSection *getStaticDtorSection(unsigned Priority,
                                  const MCSymbol *KeySym) const override;

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
                                   Align &Alignment,
                                   const Function *F) const override;

  MCSection *getSectionForConstant(const DataLayout &DL, SectionKind Kind,
                                   const Constant *C, Align &Alignment,
                                   const Function *F,
                                   StringRef SectionSuffix) const override;

  MCSection *getSectionForJumpTable(const Function &F,
                                    const TargetMachine &TM) const override;

  MCSection *
  getSectionForJumpTable(const Function &F, const TargetMachine &TM,
                         const MachineJumpTableEntry *JTE) const override;

  MCSection *getSectionForLSDA(const Function &F, const MCSymbol &FnSym,
                               const TargetMachine &TM) const override;

  MCSection *getExceptionSitesSection(const Function &F, const MCSymbol &FnSym,
                                      const TargetMachine &TM) const override;

  // Whether an object is placed in the flash window without an explicit
  // section. The cost model uses the same predicate, so a load's cost matches
  // where the object is placed.
  static bool isAutoFlashGlobal(const GlobalObject *GO);

private:
  enum class AVMSectionClass { Data, ProgramData, ProgramText, Flash };

  MCSection *moveToFlash(MCSection *Section) const;
  SectionKind getAVMSectionKind(const GlobalObject *GO, SectionKind Kind) const;
  MCSection *classifySection(MCSection *Section, AVMSectionClass Class) const;
};
} // namespace llvm

#endif
