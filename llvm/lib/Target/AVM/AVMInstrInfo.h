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

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, Register DestReg, Register SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;
  void storeRegToStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
      bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
  void loadRegFromStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
      int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      unsigned SubReg = 0,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
};

} // namespace llvm

#endif
