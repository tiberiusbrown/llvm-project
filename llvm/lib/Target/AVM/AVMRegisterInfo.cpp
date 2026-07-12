#include "AVMRegisterInfo.h"
#include "AVMFrameLowering.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
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
  return AVM::R3;
}

bool AVMRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator, int,
                                          unsigned, RegScavenger *) const {
  report_fatal_error(
      "AVM frame-index elimination is not implemented before GlobalISel");
}
