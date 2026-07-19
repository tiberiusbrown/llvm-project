//===-- AVMSubtarget.h - AVM subtarget information ------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMSUBTARGET_H
#define LLVM_LIB_TARGET_AVM_AVMSUBTARGET_H

#include "AVMFrameLowering.h"
#include "AVMISelLowering.h"
#include "AVMInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "AVMGenSubtargetInfo.inc"

namespace llvm {
class AVMSelectionDAGInfo;

class AVMSubtarget final : public AVMGenSubtargetInfo {
  bool IsV1 = false;
  AVMInstrInfo InstrInfo;
  AVMTargetLowering TLInfo;
  AVMFrameLowering FrameLowering;
  std::unique_ptr<const SelectionDAGTargetInfo> TSInfo;

  AVMSubtarget &initializeSubtargetDependencies(StringRef CPU,
                                                StringRef TuneCPU,
                                                StringRef FS);

public:
  AVMSubtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU, StringRef FS,
               const TargetMachine &TM);
  ~AVMSubtarget() override;

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const AVMInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const AVMRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const AVMTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const AVMFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const SelectionDAGTargetInfo *getSelectionDAGInfo() const override {
    return TSInfo.get();
  }
};
} // namespace llvm

#endif
