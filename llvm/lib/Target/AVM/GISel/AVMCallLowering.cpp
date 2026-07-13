#include "AVMCallLowering.h"
#include "../AVMISelLowering.h"
#include "../AVMInstrInfo.h"
#include "../MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Function.h"

using namespace llvm;

AVMCallLowering::AVMCallLowering(const AVMTargetLowering &TLI)
    : CallLowering(&TLI) {}

static Register argumentRegister(unsigned Index, unsigned Width) {
  static const Register Words[] = {AVM::R4, AVM::R5, AVM::R6, AVM::R7};
  static const Register Bytes[] = {AVM::B4, AVM::B5, AVM::B6, AVM::B7};
  return Width <= 8 ? Bytes[Index] : Words[Index];
}

bool AVMCallLowering::lowerFormalArguments(MachineIRBuilder &MIRBuilder,
                                           const Function &F,
                                           ArrayRef<ArrayRef<Register>> VRegs,
                                           FunctionLoweringInfo &) const {
  if (F.isVarArg() || VRegs.size() > 4)
    return false;

  MachineRegisterInfo &MRI = MIRBuilder.getMF().getRegInfo();
  for (unsigned I = 0; I != VRegs.size(); ++I) {
    if (VRegs[I].size() != 1)
      return false;
    Register VReg = VRegs[I][0];
    unsigned Width = MRI.getType(VReg).getSizeInBits();
    if (Width == 0 || Width > 16)
      return false;
    Register Phys = argumentRegister(I, Width);
    MRI.addLiveIn(Phys);
    MIRBuilder.getMBB().addLiveIn(Phys);
    MIRBuilder.buildCopy(VReg, Phys);
  }
  return true;
}

bool AVMCallLowering::lowerReturn(MachineIRBuilder &MIRBuilder, const Value *,
                                  ArrayRef<Register> VRegs,
                                  FunctionLoweringInfo &, Register) const {
  auto Ret = MIRBuilder.buildInstrNoInsert(AVM::RET);
  if (!VRegs.empty()) {
    if (VRegs.size() != 1)
      return false;
    Register VReg = VRegs[0];
    unsigned Width =
        MIRBuilder.getMF().getRegInfo().getType(VReg).getSizeInBits();
    if (Width == 0 || Width > 16)
      return false;

    const Function &F = MIRBuilder.getMF().getFunction();
    Register Phys = argumentRegister(0, Width);
    if (Width <= 8 && (F.getAttributes().hasRetAttr(Attribute::SExt) ||
                       F.getAttributes().hasRetAttr(Attribute::ZExt))) {
      MachineRegisterInfo &MRI = MIRBuilder.getMF().getRegInfo();
      Register Extended = MRI.createGenericVirtualRegister(LLT::scalar(16));
      unsigned Opcode = F.getAttributes().hasRetAttr(Attribute::SExt)
                            ? TargetOpcode::G_SEXT
                            : TargetOpcode::G_ZEXT;
      MIRBuilder.buildInstr(Opcode, {Extended}, {VReg});
      Phys = AVM::R4;
      MIRBuilder.buildCopy(Phys, Extended);
    } else {
      MIRBuilder.buildCopy(Phys, VReg);
    }
    Ret.addUse(Phys, RegState::Implicit);
  }
  MIRBuilder.insertInstr(Ret);
  return true;
}

bool AVMCallLowering::lowerCall(MachineIRBuilder &, CallLoweringInfo &) const {
  return false;
}
