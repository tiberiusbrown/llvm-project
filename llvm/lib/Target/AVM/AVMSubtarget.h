#ifndef LLVM_LIB_TARGET_AVM_AVMSUBTARGET_H
#define LLVM_LIB_TARGET_AVM_AVMSUBTARGET_H

#include "AVMFrameLowering.h"
#include "AVMInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "AVMGenSubtargetInfo.inc"

namespace llvm {

class AVMSubtarget final : public AVMGenSubtargetInfo {
  bool IsV1 = true;
  AVMInstrInfo InstrInfo;
  AVMFrameLowering FrameLowering;

public:
  AVMSubtarget(const Triple &TT, StringRef CPU, StringRef FS);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const AVMInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const AVMRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const AVMFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
};

} // namespace llvm

#endif
