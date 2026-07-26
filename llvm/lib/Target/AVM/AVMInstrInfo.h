//===-- AVMInstrInfo.h - AVM instruction information ----------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMINSTRINFO_H
#define LLVM_LIB_TARGET_AVM_AVMINSTRINFO_H

#include "AVMRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "AVMGenInstrInfo.inc"

namespace llvm {
class AVMSubtarget;

namespace AVMII {
enum TargetOperandFlags : unsigned {
  MO_NONE,
  MO_LO16,
  MO_HI8,
};
} // namespace AVMII

class AVMInstrInfo final : public AVMGenInstrInfo {
  AVMRegisterInfo RI;

public:
  explicit AVMInstrInfo(const AVMSubtarget &STI);

  const AVMRegisterInfo &getRegisterInfo() const { return RI; }

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, Register DestReg, Register SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;
  bool isProfitableToFoldRedundantCopy(
      const MachineInstr &PrevCopy, const MachineInstr &Copy,
      const MachineRegisterInfo &MRI) const override;

  void storeRegToStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
      bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
  void loadRegFromStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
      int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      unsigned SubReg = 0,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;

  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify = false) const override;
  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;
  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL,
                        int *BytesAdded = nullptr) const override;
  bool
  reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;

  bool isPredicated(const MachineInstr &MI) const override;
  bool isPredicable(const MachineInstr &MI) const override;
  bool PredicateInstruction(MachineInstr &MI,
                            ArrayRef<MachineOperand> Pred) const override;
  bool ClobbersPredicate(MachineInstr &MI, std::vector<MachineOperand> &Pred,
                         bool SkipDead) const override;
  bool isProfitableToIfCvt(MachineBasicBlock &MBB, unsigned NumCycles,
                           unsigned ExtraPredCycles,
                           BranchProbability Probability) const override;
  bool isProfitableToIfCvt(MachineBasicBlock &TMBB, unsigned NumTCycles,
                           unsigned ExtraTCycles, MachineBasicBlock &FMBB,
                           unsigned NumFCycles, unsigned ExtraFCycles,
                           BranchProbability Probability) const override;

  unsigned getInstrLatency(const InstrItineraryData *ItinData,
                           const MachineInstr &MI,
                           unsigned *PredCost = nullptr) const override;
  unsigned getInstSizeInBytes(const MachineInstr &MI) const override;
  bool isReMaterializableImpl(const MachineInstr &MI) const override;
};
} // namespace llvm

#endif
