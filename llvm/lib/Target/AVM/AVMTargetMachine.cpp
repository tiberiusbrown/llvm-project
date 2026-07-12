#include "AVMTargetMachine.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

static Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::Static);
}

AVMTargetMachine::AVMTargetMachine(
    const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
    const TargetOptions &Options, std::optional<Reloc::Model> RM,
    std::optional<CodeModel::Model> CM, CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(
          T, TT.computeDataLayout(), TT, CPU, FS, Options,
          getEffectiveRelocModel(RM),
          getEffectiveCodeModel(CM, CodeModel::Large), OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, CPU, FS) {
  if (JIT)
    report_fatal_error("AVM does not support JIT code generation");
  initAsmInfo();
}

AVMTargetMachine::~AVMTargetMachine() = default;

namespace {
class AVMPassConfig final : public TargetPassConfig {
public:
  AVMPassConfig(AVMTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  bool addInstSelector() override {
    report_fatal_error(
        "AVM instruction selection is not implemented; MC assembly is available");
  }
};
} // namespace

TargetPassConfig *
AVMTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new AVMPassConfig(*this, PM);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeAVMTarget() {
  RegisterTargetMachine<AVMTargetMachine> X(getTheAVMTarget());
}
