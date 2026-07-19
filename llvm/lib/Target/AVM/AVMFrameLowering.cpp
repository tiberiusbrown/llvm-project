//===-- AVMFrameLowering.cpp - AVM frame lowering ------------------------===//

#include "AVMFrameLowering.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

AVMFrameLowering::AVMFrameLowering()
    : TargetFrameLowering(StackGrowsDown, Align(1), 0, Align(1)) {}

bool AVMFrameLowering::hasFPImpl(const MachineFunction &) const {
  return false;
}

void AVMFrameLowering::emitPrologue(MachineFunction &MF,
                                    MachineBasicBlock &) const {
  if (MF.getFrameInfo().getStackSize())
    report_fatal_error("AVM stack frames are not implemented yet");
}

void AVMFrameLowering::emitEpilogue(MachineFunction &MF,
                                    MachineBasicBlock &) const {
  if (MF.getFrameInfo().getStackSize())
    report_fatal_error("AVM stack frames are not implemented yet");
}
