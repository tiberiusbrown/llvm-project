//===-- AVMFrameLowering.cpp - AVM frame lowering ------------------------===//

#include "AVMFrameLowering.h"
#include "AVM.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

#include <algorithm>

using namespace llvm;

AVMFrameLowering::AVMFrameLowering()
    : TargetFrameLowering(StackGrowsDown, Align(1), 0, Align(1)) {}

namespace {
void emitSPAdjustment(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                      const DebugLoc &DL, const AVMInstrInfo &TII,
                      int64_t Amount, MachineInstr::MIFlag Flag) {
  while (Amount) {
    int64_t Chunk = std::clamp<int64_t>(Amount, -128, 127);
    BuildMI(MBB, MI, DL, TII.get(AVM::ADJSP)).addImm(Chunk).setMIFlag(Flag);
    Amount -= Chunk;
  }
}

uint64_t getCalleeSavedSize(const MachineFrameInfo &MFI) {
  return MFI.getCalleeSavedInfo().size() * 2;
}
} // namespace

bool AVMFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MFI.isFrameAddressTaken() || MFI.hasVarSizedObjects();
}

void AVMFrameLowering::emitPrologue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const {
  assert(&MF.front() == &MBB && "AVM shrink wrapping is not implemented");
  MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.hasVarSizedObjects())
    report_fatal_error("dynamic AVM stack allocation is unsupported");
  const AVMInstrInfo &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();

  MachineBasicBlock::iterator MI = MBB.begin();
  while (MI != MBB.end() && MI->getOpcode() == AVM::PUSH16 &&
         MI->getFlag(MachineInstr::FrameSetup))
    ++MI;
  DebugLoc DL = MI == MBB.end() ? DebugLoc() : MI->getDebugLoc();

  uint64_t CalleeSavedSize = getCalleeSavedSize(MFI);
  assert(MFI.getStackSize() >= CalleeSavedSize &&
         "invalid AVM callee-save frame size");
  uint64_t FixedSize = MFI.getStackSize() - CalleeSavedSize;
  emitSPAdjustment(MBB, MI, DL, TII, -static_cast<int64_t>(FixedSize),
                   MachineInstr::FrameSetup);

  if (hasFP(MF)) {
    BuildMI(MBB, MI, DL, TII.get(AVM::GETSP), AVM::R3)
        .setMIFlag(MachineInstr::FrameSetup);
    for (MachineBasicBlock &Block : llvm::drop_begin(MF))
      Block.addLiveIn(AVM::R3);
  }
}

void AVMFrameLowering::emitEpilogue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const AVMInstrInfo &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
  MachineBasicBlock::iterator MI = MBB.getFirstTerminator();
  while (MI != MBB.begin()) {
    MachineBasicBlock::iterator Prev = std::prev(MI);
    if (Prev->getOpcode() != AVM::POP16 ||
        !Prev->getFlag(MachineInstr::FrameDestroy))
      break;
    MI = Prev;
  }
  DebugLoc DL = MI == MBB.end() ? DebugLoc() : MI->getDebugLoc();

  if (hasFP(MF))
    BuildMI(MBB, MI, DL, TII.get(AVM::SETSP))
        .addReg(AVM::R3)
        .setMIFlag(MachineInstr::FrameDestroy);

  uint64_t CalleeSavedSize = getCalleeSavedSize(MFI);
  assert(MFI.getStackSize() >= CalleeSavedSize &&
         "invalid AVM callee-save frame size");
  emitSPAdjustment(MBB, MI, DL, TII,
                   static_cast<int64_t>(MFI.getStackSize() - CalleeSavedSize),
                   MachineInstr::FrameDestroy);
}

bool AVMFrameLowering::spillCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    ArrayRef<CalleeSavedInfo> CSI, const TargetRegisterInfo *) const {
  if (CSI.empty())
    return false;
  const AVMInstrInfo &TII =
      *MBB.getParent()->getSubtarget<AVMSubtarget>().getInstrInfo();
  DebugLoc DL = MI == MBB.end() ? DebugLoc() : MI->getDebugLoc();
  for (const CalleeSavedInfo &Info : llvm::reverse(CSI)) {
    MBB.addLiveIn(Info.getReg());
    BuildMI(MBB, MI, DL, TII.get(AVM::PUSH16))
        .addReg(Info.getReg(), RegState::Kill)
        .setMIFlag(MachineInstr::FrameSetup);
  }
  return true;
}

bool AVMFrameLowering::restoreCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    MutableArrayRef<CalleeSavedInfo> CSI, const TargetRegisterInfo *) const {
  if (CSI.empty())
    return false;
  const AVMInstrInfo &TII =
      *MBB.getParent()->getSubtarget<AVMSubtarget>().getInstrInfo();
  DebugLoc DL = MI == MBB.end() ? DebugLoc() : MI->getDebugLoc();
  for (const CalleeSavedInfo &Info : CSI)
    BuildMI(MBB, MI, DL, TII.get(AVM::POP16), Info.getReg())
        .setMIFlag(MachineInstr::FrameDestroy);
  return true;
}

void AVMFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                            BitVector &SavedRegs,
                                            RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);
  if (hasFP(MF))
    SavedRegs.set(AVM::R3);
}

MachineBasicBlock::iterator AVMFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI) const {
  const AVMInstrInfo &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
  int64_t Amount = TII.getFrameSize(*MI);
  if (MI->getOpcode() == TII.getCallFrameSetupOpcode())
    Amount = -Amount;
  else {
    assert(MI->getOpcode() == TII.getCallFrameDestroyOpcode() &&
           "invalid AVM call-frame pseudo");
  }
  emitSPAdjustment(MBB, MI, MI->getDebugLoc(), TII, Amount,
                   MachineInstr::NoFlags);
  return MBB.erase(MI);
}

void AVMFrameLowering::processFunctionBeforeFrameFinalized(
    MachineFunction &MF, RegScavenger *RS) const {
  assert(RS && "AVM requires register scavenging");
  MachineFrameInfo &MFI = MF.getFrameInfo();
  if (hasFP(MF) || MFI.estimateStackSize(MF) > 255)
    RS->addScavengingFrameIndex(MFI.CreateSpillStackObject(2, Align(1)));
}

bool AVMFrameLowering::needsFrameIndexResolution(
    const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MFI.hasStackObjects() || MFI.adjustsStack();
}
