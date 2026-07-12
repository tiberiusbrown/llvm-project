#ifndef LLVM_LIB_TARGET_AVM_AVMREGISTERINFO_H
#define LLVM_LIB_TARGET_AVM_AVMREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "AVMGenRegisterInfo.inc"

namespace llvm {

class AVMRegisterInfo final : public AVMGenRegisterInfo {
public:
  AVMRegisterInfo();

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const override;
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  Register getFrameRegister(const MachineFunction &MF) const override;
  bool eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;
};

} // namespace llvm

#endif
