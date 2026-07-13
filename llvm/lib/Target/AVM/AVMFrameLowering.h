#ifndef LLVM_LIB_TARGET_AVM_AVMFRAMELOWERING_H
#define LLVM_LIB_TARGET_AVM_AVMFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class AVMFrameLowering final : public TargetFrameLowering {
public:
  AVMFrameLowering()
      : TargetFrameLowering(StackGrowsDown, Align(1), 0, Align(1)) {}

  void emitPrologue(MachineFunction &, MachineBasicBlock &) const override;
  void emitEpilogue(MachineFunction &, MachineBasicBlock &) const override;

private:
  bool hasFPImpl(const MachineFunction &) const override { return false; }
};

} // namespace llvm

#endif
