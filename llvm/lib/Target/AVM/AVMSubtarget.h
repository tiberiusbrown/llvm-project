#ifndef LLVM_LIB_TARGET_AVM_AVMSUBTARGET_H
#define LLVM_LIB_TARGET_AVM_AVMSUBTARGET_H

#include "AVMFrameLowering.h"
#include "AVMISelLowering.h"
#include "AVMInstrInfo.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/RegisterBankInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "AVMGenSubtargetInfo.inc"

namespace llvm {

class AVMTargetMachine;

class AVMSubtarget final : public AVMGenSubtargetInfo {
  bool IsV1 = true;
  AVMInstrInfo InstrInfo;
  AVMFrameLowering FrameLowering;
  AVMTargetLowering TLInfo;
  std::unique_ptr<CallLowering> CallLoweringInfo;
  std::unique_ptr<InstructionSelector> InstSelector;
  std::unique_ptr<LegalizerInfo> Legalizer;
  std::unique_ptr<RegisterBankInfo> RegBankInfo;

public:
  AVMSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
               const AVMTargetMachine &TM);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const AVMInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const AVMRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const AVMFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const AVMTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const CallLowering *getCallLowering() const override;
  InstructionSelector *getInstructionSelector() const override;
  const LegalizerInfo *getLegalizerInfo() const override;
  const RegisterBankInfo *getRegBankInfo() const override;
};

} // namespace llvm

#endif
