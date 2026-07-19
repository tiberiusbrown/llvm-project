//===-- AVMTargetMachine.h - AVM target machine ----------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMTARGETMACHINE_H
#define LLVM_LIB_TARGET_AVM_AVMTARGETMACHINE_H

#include "AVMSubtarget.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"

namespace llvm {
class AVMTargetMachine final : public CodeGenTargetMachineImpl {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  mutable StringMap<std::unique_ptr<AVMSubtarget>> SubtargetMap;

public:
  AVMTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                   StringRef FS, const TargetOptions &Options,
                   std::optional<Reloc::Model> RM,
                   std::optional<CodeModel::Model> CM, CodeGenOptLevel OL,
                   bool JIT);
  ~AVMTargetMachine() override;

  const AVMSubtarget *getSubtargetImpl(const Function &F) const override;
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
  MachineFunctionInfo *
  createMachineFunctionInfo(BumpPtrAllocator &Allocator, const Function &F,
                            const TargetSubtargetInfo *STI) const override;
};
} // namespace llvm

#endif
