//===-- AVMFrameLowering.h - AVM frame lowering ---------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMFRAMELOWERING_H
#define LLVM_LIB_TARGET_AVM_AVMFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class AVMFrameLowering final : public TargetFrameLowering {
public:
  AVMFrameLowering();

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

protected:
  bool hasFPImpl(const MachineFunction &MF) const override;
};
} // namespace llvm

#endif
