//===-- ABCInstrInfo.h - Instruction info for ABC --------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_ABCINSTRINFO_H
#define LLVM_LIB_TARGET_ABC_ABCINSTRINFO_H

#include "ABCRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "ABCGenInstrInfo.inc"

namespace llvm {
class ABCSubtarget;

class ABCInstrInfo : public ABCGenInstrInfo {
  const ABCRegisterInfo RI;

public:
  explicit ABCInstrInfo(const ABCSubtarget &STI);
  const ABCRegisterInfo &getRegisterInfo() const { return RI; }
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_ABCINSTRINFO_H
