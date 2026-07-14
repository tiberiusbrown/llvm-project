//===-- ABCSubtarget.cpp - Subtarget information for ABC ------------------===//

#include "ABCSubtarget.h"
#include "ABCTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "abc-subtarget"

#define GET_SUBTARGETINFO_CTOR
#define GET_SUBTARGETINFO_TARGET_DESC
#include "ABCGenSubtargetInfo.inc"

ABCSubtarget::ABCSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                           const ABCTargetMachine &TM)
    : ABCGenSubtargetInfo(TT, CPU.empty() ? "generic" : CPU,
                          CPU.empty() ? "generic" : CPU, FS),
      InstrInfo(*this), FrameLowering(*this), TLInfo(TM, *this) {}

void ABCSubtarget::anchor() {}
