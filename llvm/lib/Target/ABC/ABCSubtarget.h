//===-- ABCSubtarget.h - Subtarget information for ABC ---------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_ABCSUBTARGET_H
#define LLVM_LIB_TARGET_ABC_ABCSUBTARGET_H

#include "ABCFrameLowering.h"
#include "ABCInstrInfo.h"
#include "ABCTargetLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "ABCGenSubtargetInfo.inc"

namespace llvm {
class ABCTargetMachine;

class ABCSubtarget : public ABCGenSubtargetInfo {
  ABCInstrInfo InstrInfo;
  ABCFrameLowering FrameLowering;
  ABCTargetLowering TLInfo;

  virtual void anchor();

public:
  ABCSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
               const ABCTargetMachine &TM);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const ABCInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const ABCRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const ABCFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const ABCTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_ABCSUBTARGET_H
