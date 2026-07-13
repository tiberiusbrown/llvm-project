#include "AVMSubtarget.h"
#include "AVMTargetMachine.h"
#include "GISel/AVMCallLowering.h"
#include "GISel/AVMInstructionSelector.h"
#include "GISel/AVMLegalizerInfo.h"
#include "GISel/AVMRegisterBankInfo.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"

using namespace llvm;

#define DEBUG_TYPE "avm-subtarget"
#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AVMGenSubtargetInfo.inc"

AVMSubtarget::AVMSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                           const AVMTargetMachine &TM)
    : AVMGenSubtargetInfo(TT, CPU.empty() ? "generic" : CPU,
                          CPU.empty() ? "generic" : CPU, FS),
      InstrInfo(*this), TLInfo(TM, *this) {
  StringRef EffectiveCPU = CPU.empty() ? "generic" : CPU;
  ParseSubtargetFeatures(EffectiveCPU, EffectiveCPU, FS);
  CallLoweringInfo = std::make_unique<AVMCallLowering>(TLInfo);
  Legalizer = std::make_unique<AVMLegalizerInfo>(*this);
  auto *RBI = new AVMRegisterBankInfo(*getRegisterInfo());
  RegBankInfo.reset(RBI);
  InstSelector.reset(createAVMInstructionSelector(TM, *this, *RBI));
}

const CallLowering *AVMSubtarget::getCallLowering() const {
  return CallLoweringInfo.get();
}
InstructionSelector *AVMSubtarget::getInstructionSelector() const {
  return InstSelector.get();
}
const LegalizerInfo *AVMSubtarget::getLegalizerInfo() const {
  return Legalizer.get();
}
const RegisterBankInfo *AVMSubtarget::getRegBankInfo() const {
  return RegBankInfo.get();
}
