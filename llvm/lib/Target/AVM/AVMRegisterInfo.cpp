#include "AVMRegisterInfo.h"
#include "AVMFrameLowering.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "AVMGenRegisterInfo.inc"

AVMRegisterInfo::AVMRegisterInfo() : AVMGenRegisterInfo(0) {}

const MCPhysReg *
AVMRegisterInfo::getCalleeSavedRegs(const MachineFunction *) const {
  return CSR_AVM_SaveList;
}

const uint32_t *AVMRegisterInfo::getCallPreservedMask(const MachineFunction &,
                                                      CallingConv::ID) const {
  return CSR_AVM_RegMask;
}

BitVector AVMRegisterInfo::getReservedRegs(const MachineFunction &) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(AVM::SP);
  Reserved.set(AVM::FLAGS);
  Reserved.set(AVM::PB);
  Reserved.set(AVM::CB);
  return Reserved;
}

Register AVMRegisterInfo::getFrameRegister(const MachineFunction &) const {
  return AVM::SP;
}

bool AVMRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                          int SPAdj, unsigned FIOperandNum,
                                          RegScavenger *) const {
  MachineFunction &MF = *MI->getParent()->getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  int FI = MI->getOperand(FIOperandNum).getIndex();
  int64_t Offset = MFI.getObjectOffset(FI) + MFI.getStackSize() + SPAdj;
  // Ordinary stack instructions encode an unsigned byte displacement.  A
  // materialized frame address uses ADDI16, which is also how incoming stack
  // arguments beyond a nearly full local frame remain addressable.
  const int64_t MaxOffset = MI->getOpcode() == AVM::ADDI16 ? 65535 : 255;
  if (Offset < 0 || Offset > MaxOffset)
    report_fatal_error("AVM stack slot offset is outside the encodable range");
  MI->getOperand(FIOperandNum).ChangeToImmediate(Offset);
  return false;
}
