//===-- ABCFrameLowering.h - Frame lowering for ABC ------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_ABCFRAMELOWERING_H
#define LLVM_LIB_TARGET_ABC_ABCFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class ABCSubtarget;

class ABCFrameLowering : public TargetFrameLowering {
public:
  explicit ABCFrameLowering(const ABCSubtarget &STI)
      : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(1), 0) {}

  void emitPrologue(MachineFunction &, MachineBasicBlock &) const override {}
  void emitEpilogue(MachineFunction &, MachineBasicBlock &) const override {}

protected:
  bool hasFPImpl(const MachineFunction &) const override { return false; }
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_ABCFRAMELOWERING_H
