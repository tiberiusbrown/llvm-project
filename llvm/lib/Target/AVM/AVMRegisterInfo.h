//===-- AVMRegisterInfo.h - AVM register information ----------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMREGISTERINFO_H
#define LLVM_LIB_TARGET_AVM_AVMREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "AVMGenRegisterInfo.inc"

namespace llvm {
namespace AVMRI {
enum RegAllocHint : unsigned {
  SpriteRun = 1,
};
} // namespace AVMRI

class AVMRegisterInfo final : public AVMGenRegisterInfo {
public:
  AVMRegisterInfo();

  const uint16_t *
  getCalleeSavedRegs(const MachineFunction *MF = nullptr) const override;
  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const override;
  bool requiresRegisterScavenging(const MachineFunction &) const override {
    return true;
  }
  bool requiresFrameIndexReplacementScavenging(
      const MachineFunction &) const override {
    return true;
  }
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool getRegAllocationHints(Register VirtReg, ArrayRef<MCPhysReg> Order,
                             SmallVectorImpl<MCPhysReg> &Hints,
                             const MachineFunction &MF,
                             const VirtRegMap *VRM = nullptr,
                             const LiveRegMatrix *Matrix = nullptr) const override;
  bool shouldCoalesce(MachineInstr *MI, const TargetRegisterClass *SrcRC,
                      unsigned SubReg, const TargetRegisterClass *DstRC,
                      unsigned DstSubReg, const TargetRegisterClass *NewRC,
                      LiveIntervals &LIS) const override;
  bool eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;
  Register getFrameRegister(const MachineFunction &MF) const override;
  const TargetRegisterClass *
  getPointerRegClass(unsigned Kind = 0) const override;
};
} // namespace llvm

#endif
