//===-- ABCRegisterInfo.h - Register info for ABC --------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_ABCREGISTERINFO_H
#define LLVM_LIB_TARGET_ABC_ABCREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "ABCGenRegisterInfo.inc"

namespace llvm {
class ABCRegisterInfo : public ABCGenRegisterInfo {
public:
  ABCRegisterInfo();

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;
  Register getFrameRegister(const MachineFunction &MF) const override;
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_ABCREGISTERINFO_H
