//===-- AVMSubtarget.cpp - AVM subtarget information ---------------------===//

#include "AVMSubtarget.h"
#include "AVMSelectionDAGInfo.h"

using namespace llvm;

#define DEBUG_TYPE "avm-subtarget"
#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AVMGenSubtargetInfo.inc"

AVMSubtarget &AVMSubtarget::initializeSubtargetDependencies(StringRef CPU,
                                                            StringRef TuneCPU,
                                                            StringRef FS) {
  if (CPU.empty())
    CPU = "avm1";
  ParseSubtargetFeatures(CPU, TuneCPU, FS);
  return *this;
}

AVMSubtarget::AVMSubtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU,
                           StringRef FS, const TargetMachine &TM)
    : AVMGenSubtargetInfo(TT, CPU, TuneCPU, FS),
      InstrInfo(initializeSubtargetDependencies(CPU, TuneCPU, FS)),
      TLInfo(TM, *this), FrameLowering(),
      TSInfo(std::make_unique<AVMSelectionDAGInfo>()) {}

AVMSubtarget::~AVMSubtarget() = default;
