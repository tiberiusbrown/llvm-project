#ifndef LLVM_LIB_TARGET_AVM_AVMTARGETMACHINE_H
#define LLVM_LIB_TARGET_AVM_AVMTARGETMACHINE_H

#include "AVMSubtarget.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include <memory>
#include <optional>

namespace llvm {

class AVMTargetMachine final : public CodeGenTargetMachineImpl {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  AVMSubtarget Subtarget;

public:
  AVMTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                   StringRef FS, const TargetOptions &Options,
                   std::optional<Reloc::Model> RM,
                   std::optional<CodeModel::Model> CM, CodeGenOptLevel OL,
                   bool JIT);
  ~AVMTargetMachine() override;

  const AVMSubtarget *getSubtargetImpl(const Function &) const override {
    return &Subtarget;
  }
  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
};

} // namespace llvm

#endif
