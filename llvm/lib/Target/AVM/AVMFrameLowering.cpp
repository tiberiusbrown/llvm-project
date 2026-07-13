#include "AVMFrameLowering.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"
#include <algorithm>

using namespace llvm;

static void emitAdjustment(MachineFunction &MF, MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator Where, int Amount,
                           MachineInstr::MIFlag Flag) {
  const auto &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
  while (Amount) {
    int Chunk = std::clamp(Amount, -127, 127);
    BuildMI(MBB, Where, DebugLoc(), TII.get(AVM::ADJSP))
        .addImm(Chunk)
        .setMIFlag(Flag);
    Amount -= Chunk;
  }
}

void AVMFrameLowering::emitPrologue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const {
  uint64_t Size = MF.getFrameInfo().getStackSize();
  if (Size > 256)
    report_fatal_error("AVM fixed stack frame exceeds the 256-byte VM stack");
  emitAdjustment(MF, MBB, MBB.begin(), -static_cast<int>(Size),
                 MachineInstr::FrameSetup);
}

void AVMFrameLowering::emitEpilogue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const {
  uint64_t Size = MF.getFrameInfo().getStackSize();
  auto Where = MBB.getFirstTerminator();
  emitAdjustment(MF, MBB, Where, static_cast<int>(Size),
                 MachineInstr::FrameDestroy);
}

MachineBasicBlock::iterator AVMFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI) const {
  int Amount = static_cast<int>(MI->getOperand(0).getImm());
  bool IsSetup = MI->getOpcode() == AVM::ADJCALLSTACKDOWN;
  auto Next = std::next(MI);
  emitAdjustment(MF, MBB, MI, IsSetup ? -Amount : Amount,
                 IsSetup ? MachineInstr::FrameSetup
                         : MachineInstr::FrameDestroy);
  MI->eraseFromParent();
  return Next;
}
