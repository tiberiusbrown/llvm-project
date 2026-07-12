#include "AVMSubtarget.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"

using namespace llvm;

#define DEBUG_TYPE "avm-subtarget"
#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AVMGenSubtargetInfo.inc"

AVMSubtarget::AVMSubtarget(const Triple &TT, StringRef CPU, StringRef FS)
    : AVMGenSubtargetInfo(TT, CPU.empty() ? "generic" : CPU,
                          CPU.empty() ? "generic" : CPU, FS),
      InstrInfo(*this) {
  StringRef EffectiveCPU = CPU.empty() ? "generic" : CPU;
  ParseSubtargetFeatures(EffectiveCPU, EffectiveCPU, FS);
}
