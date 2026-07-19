//===-- AVMRegisterInfo.cpp - AVM register information -------------------===//

#include "AVMRegisterInfo.h"
#include "AVM.h"
#include "AVMFrameLowering.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "AVMGenRegisterInfo.inc"

AVMRegisterInfo::AVMRegisterInfo() : AVMGenRegisterInfo(0) {}

const uint16_t *
AVMRegisterInfo::getCalleeSavedRegs(const MachineFunction *) const {
  return CSR_AVM_SaveList;
}

const uint32_t *AVMRegisterInfo::getCallPreservedMask(const MachineFunction &,
                                                      CallingConv::ID) const {
  return CSR_AVM_RegMask;
}

BitVector AVMRegisterInfo::getReservedRegs(const MachineFunction &) const {
  BitVector Reserved(getNumRegs());
  markSuperRegs(Reserved, AVM::SP);
  markSuperRegs(Reserved, AVM::PC);
  markSuperRegs(Reserved, AVM::CC);
  return Reserved;
}

bool AVMRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI, int,
                                          unsigned, RegScavenger *) const {
  MI->getParent()->getParent()->getFunction().getContext().emitError(
      "AVM frame indices are not supported by minimal leaf code generation");
  return false;
}

Register AVMRegisterInfo::getFrameRegister(const MachineFunction &) const {
  return AVM::SP;
}

const TargetRegisterClass *
AVMRegisterInfo::getPointerRegClass(unsigned Kind) const {
  assert(Kind == 0 && "AVM has no alternate pointer register class yet");
  return &AVM::PTR16RegClass;
}
