//===-- ABCTargetMachine.cpp - Target machine for ABC ---------------------===//

#include "ABCTargetMachine.h"
#include "ABC.h"
#include "ABCTargetTransformInfo.h"
#include "TargetInfo/ABCTargetInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Target/TargetLoweringObjectFile.h"

using namespace llvm;

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeABCTarget() {
  RegisterTargetMachine<ABCTargetMachine> X(getTheABCTarget());
}

namespace {
class ABCPassConfig : public TargetPassConfig {
public:
  ABCPassConfig(ABCTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  bool addInstSelector() override {
    report_fatal_error("ABC uses the target-local IR translator pipeline");
  }
};
} // namespace

ABCTargetMachine::ABCTargetMachine(
    const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
    const TargetOptions &Options, std::optional<Reloc::Model> RM,
    std::optional<CodeModel::Model> CM, CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, TT.computeDataLayout(), TT,
                               CPU.empty() ? "generic" : CPU, FS, Options,
                               Reloc::Static, CodeModel::Small, OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(std::make_unique<ABCSubtarget>(TT, CPU, FS, *this)) {
  initAsmInfo();
}

ABCTargetMachine::~ABCTargetMachine() = default;

const ABCSubtarget *
ABCTargetMachine::getSubtargetImpl(const Function &) const {
  return Subtarget.get();
}

TargetTransformInfo
ABCTargetMachine::getTargetTransformInfo(const Function &F) const {
  return TargetTransformInfo(std::make_unique<ABCTTIImpl>(this, F));
}

TargetPassConfig *ABCTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new ABCPassConfig(*this, PM);
}

bool ABCTargetMachine::addPassesToEmitFile(
    PassManagerBase &PM, raw_pwrite_stream &Out, raw_pwrite_stream *DwoOut,
    CodeGenFileType FileType, bool DisableVerify,
    MachineModuleInfoWrapperPass *MMIWP) {
  (void)DisableVerify;
  (void)MMIWP;

  if (DwoOut) {
    WithColor::error(errs(), "abc") << "split DWARF is not supported\n";
    return true;
  }

  auto Ctx = std::make_unique<MCContext>(getTargetTriple(), getMCAsmInfo(),
                                         getMCRegisterInfo(),
                                         getMCSubtargetInfo());
  std::unique_ptr<MCObjectFileInfo> MOFI(
      getTarget().createMCObjectFileInfo(*Ctx, /*PIC=*/false,
                                         /*LargeCodeModel=*/false));
  Ctx->setObjectFileInfo(MOFI.get());

  Expected<std::unique_ptr<MCStreamer>> StreamerOrErr =
      createMCStreamer(Out, nullptr, FileType, *Ctx);
  if (!StreamerOrErr) {
    WithColor::error(errs(), "abc")
        << toString(StreamerOrErr.takeError()) << "\n";
    return true;
  }

  PM.add(createABCIRTranslatorPass(*this, std::move(Ctx), std::move(MOFI),
                                   std::move(*StreamerOrErr)));
  return false;
}

ABCTargetLowering::ABCTargetLowering(const ABCTargetMachine &TM,
                                     const ABCSubtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i8, &ABC::STACKRegClass);
  addRegisterClass(MVT::i16, &ABC::STACKRegClass);
  addRegisterClass(MVT::i32, &ABC::STACKRegClass);
  computeRegisterProperties(STI.getRegisterInfo());
}
