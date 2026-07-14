//===-- ABCRegisterInfo.cpp - Register info for ABC -----------------------===//

#include "ABCRegisterInfo.h"
#include "ABCFrameLowering.h"
#include "MCTargetDesc/ABCMCTargetDesc.h"
#include "llvm/CodeGen/MachineFunction.h"

#define GET_REGINFO_TARGET_DESC
#include "ABCGenRegisterInfo.inc"

using namespace llvm;

ABCRegisterInfo::ABCRegisterInfo() : ABCGenRegisterInfo(ABC::STK) {}

const MCPhysReg *
ABCRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return nullptr;
}

BitVector ABCRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(ABC::STK);
  return Reserved;
}

bool ABCRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                          int SPAdj, unsigned FIOperandNum,
                                          RegScavenger *RS) const {
  return false;
}

Register ABCRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return ABC::STK;
}
