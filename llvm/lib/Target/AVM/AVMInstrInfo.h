#ifndef LLVM_LIB_TARGET_AVM_AVMINSTRINFO_H
#define LLVM_LIB_TARGET_AVM_AVMINSTRINFO_H

#include "AVMRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "AVMGenInstrInfo.inc"

namespace llvm {

class AVMSubtarget;

class AVMInstrInfo final : public AVMGenInstrInfo {
  AVMRegisterInfo RI;

public:
  explicit AVMInstrInfo(const AVMSubtarget &STI);
  const AVMRegisterInfo &getRegisterInfo() const { return RI; }
};

} // namespace llvm

#endif
