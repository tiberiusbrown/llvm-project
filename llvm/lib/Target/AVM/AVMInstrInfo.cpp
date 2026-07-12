#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "AVMGenInstrInfo.inc"

AVMInstrInfo::AVMInstrInfo(const AVMSubtarget &STI)
    : AVMGenInstrInfo(STI, RI), RI() {}
