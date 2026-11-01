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
  bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI,
                                 ArrayRef<CalleeSavedInfo> CSI,
                                 const TargetRegisterInfo *TRI) const override;
  bool
  restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MI,
                              MutableArrayRef<CalleeSavedInfo> CSI,
                              const TargetRegisterInfo *TRI) const override;
  void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs,
                            RegScavenger *RS = nullptr) const override;
  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MI) const override;
  void processFunctionBeforeFrameFinalized(
      MachineFunction &MF, RegScavenger *RS = nullptr) const override;
  void orderFrameObjects(const MachineFunction &MF,
                         SmallVectorImpl<int> &ObjectsToAllocate) const override;
  bool needsFrameIndexResolution(const MachineFunction &MF) const override;
  bool hasReservedCallFrame(const MachineFunction &) const override {
    return false;
  }

  /// Return true if the prologue changes the CFI state of the body. Functions
  /// without a frame have the entry state everywhere and need no restatement.
  bool changesFrameState(const MachineFunction &MF) const;
  /// Restate the body's CFI state at MI, the state after the prologue.
  void emitBodyFrameState(MachineBasicBlock &MBB,
                          MachineBasicBlock::iterator MI) const;
  /// Restate the state after an epilogue: the entry stack with no saved
  /// registers.
  void emitReturnFrameState(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI) const;

protected:
  bool hasFPImpl(const MachineFunction &MF) const override;
};
} // namespace llvm

#endif
