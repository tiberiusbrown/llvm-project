//===-- ABCTargetLowering.h - Target lowering for ABC ----------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_ABCTARGETLOWERING_H
#define LLVM_LIB_TARGET_ABC_ABCTARGETLOWERING_H

#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class ABCSubtarget;
class ABCTargetMachine;

class ABCTargetLowering : public TargetLowering {
public:
  ABCTargetLowering(const ABCTargetMachine &TM, const ABCSubtarget &STI);
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_ABCTARGETLOWERING_H
