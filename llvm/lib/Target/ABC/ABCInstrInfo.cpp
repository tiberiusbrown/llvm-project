//===-- ABCInstrInfo.cpp - Instruction info for ABC -----------------------===//

#include "ABCInstrInfo.h"
#include "ABCSubtarget.h"

#define GET_INSTRINFO_CTOR_DTOR
#include "ABCGenInstrInfo.inc"

using namespace llvm;

ABCInstrInfo::ABCInstrInfo(const ABCSubtarget &STI)
    : ABCGenInstrInfo(STI, RI) {}
