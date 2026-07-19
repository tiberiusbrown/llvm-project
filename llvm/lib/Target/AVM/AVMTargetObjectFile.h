//===-- AVMTargetObjectFile.h - AVM ELF section lowering ------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMTARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_AVM_AVMTARGETOBJECTFILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {
class AVMTargetObjectFile final : public TargetLoweringObjectFileELF {
  MCSection *DataSection = nullptr;
  MCSection *ProgramDataSection = nullptr;
  MCSection *ProgramTextSection = nullptr;

public:
  void Initialize(MCContext &Ctx, const TargetMachine &TM) override;
  MCSection *getExplicitSectionGlobal(const GlobalObject *GO, SectionKind Kind,
                                      const TargetMachine &TM) const override;
  MCSection *SelectSectionForGlobal(const GlobalObject *GO, SectionKind Kind,
                                    const TargetMachine &TM) const override;
};
} // namespace llvm

#endif
