//===-- ABCTargetMachine.h - Target machine for ABC ------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_ABCTARGETMACHINE_H
#define LLVM_LIB_TARGET_ABC_ABCTARGETMACHINE_H

#include "ABCSubtarget.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include <optional>

namespace llvm {
class MCContext;
class MCObjectFileInfo;
class MCStreamer;

class ABCTargetMachine : public CodeGenTargetMachineImpl {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  std::unique_ptr<ABCSubtarget> Subtarget;

public:
  ABCTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                   StringRef FS, const TargetOptions &Options,
                   std::optional<Reloc::Model> RM,
                   std::optional<CodeModel::Model> CM, CodeGenOptLevel OL,
                   bool JIT);

  ~ABCTargetMachine() override;

  const ABCSubtarget *getSubtargetImpl(const Function &) const override;
  TargetTransformInfo getTargetTransformInfo(const Function &) const override;
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  bool addPassesToEmitFile(PassManagerBase &PM, raw_pwrite_stream &Out,
                           raw_pwrite_stream *DwoOut,
                           CodeGenFileType FileType,
                           bool DisableVerify = true,
                           MachineModuleInfoWrapperPass *MMIWP = nullptr) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

ModulePass *createABCIRTranslatorPass(ABCTargetMachine &TM,
                                      std::unique_ptr<MCContext> Ctx,
                                      std::unique_ptr<MCObjectFileInfo> MOFI,
                                      std::unique_ptr<MCStreamer> Streamer);
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_ABCTARGETMACHINE_H
