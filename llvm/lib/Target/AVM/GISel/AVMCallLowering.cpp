#include "AVMCallLowering.h"
#include "../AVMISelLowering.h"
#include "../AVMInstrInfo.h"
#include "../MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineMemOperand.h"
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

static bool isSupportedScalar(LLT Ty) {
  return (Ty.isScalar() && Ty.getSizeInBits() <= 16) ||
         (Ty.isPointer() && Ty.getAddressSpace() == 0 &&
          Ty.getSizeInBits() == 16);
}

static unsigned abiSize(LLT Ty) {
  return Ty.getSizeInBits() <= 8 ? 1 : 2;
}

static Register extendByteForMemory(MachineIRBuilder &MIRBuilder,
                                    Register Value) {
  MachineRegisterInfo &MRI = MIRBuilder.getMF().getRegInfo();
  LLT Ty = MRI.getType(Value);
  if (Ty.getSizeInBits() == 16)
    return Value;
  Register Extended = MRI.createGenericVirtualRegister(LLT::scalar(16));
  MIRBuilder.buildZExt(Extended, Value);
  return Extended;
}

static Register buildIncomingStackLoad(MachineIRBuilder &MIRBuilder,
                                       Register Dst, unsigned Size,
                                       unsigned Offset) {
  MachineFunction &MF = MIRBuilder.getMF();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  int FI = MFI.CreateFixedObject(Size, 3 + Offset, true);
  Register Address = MIRBuilder
                         .buildFrameIndex(LLT::pointer(0, 16), FI)
                         .getReg(0);
  MachinePointerInfo MPO = MachinePointerInfo::getFixedStack(MF, FI);
  auto *MMO = MF.getMachineMemOperand(MPO, MachineMemOperand::MOLoad,
                                      LocationSize::precise(Size), Align(1));
  if (Size == 2) {
    MIRBuilder.buildLoad(Dst, Address, *MMO);
    return Dst;
  }
  Register Wide = MRI.createGenericVirtualRegister(LLT::scalar(16));
  MIRBuilder.buildLoad(Wide, Address, *MMO);
  MIRBuilder.buildTrunc(Dst, Wide);
  return Dst;
}

static void buildOutgoingStackStore(MachineIRBuilder &MIRBuilder,
                                    Register Value, unsigned Size,
                                    unsigned Offset) {
  MachineFunction &MF = MIRBuilder.getMF();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  Register Base = MRI.createGenericVirtualRegister(LLT::pointer(0, 16));
  MIRBuilder.buildInstr(AVM::GETSP_G).addDef(Base);
  Register Address = Base;
  if (Offset) {
    Register Off = MRI.createGenericVirtualRegister(LLT::scalar(16));
    Register Adjusted =
        MRI.createGenericVirtualRegister(LLT::pointer(0, 16));
    MIRBuilder.buildConstant(Off, Offset);
    MIRBuilder.buildPtrAdd(Adjusted, Base, Off);
    Address = Adjusted;
  }
  MachinePointerInfo MPO = MachinePointerInfo::getStack(MF, Offset);
  auto *MMO = MF.getMachineMemOperand(MPO, MachineMemOperand::MOStore,
                                      LocationSize::precise(Size), Align(1));
  MIRBuilder.buildStore(Size == 1 ? extendByteForMemory(MIRBuilder, Value)
                                 : Value,
                        Address, *MMO);
}

bool AVMCallLowering::lowerFormalArguments(MachineIRBuilder &MIRBuilder,
                                           const Function &F,
                                           ArrayRef<ArrayRef<Register>> VRegs,
                                           FunctionLoweringInfo &) const {
  if (F.isVarArg())
    return false;

  MachineRegisterInfo &MRI = MIRBuilder.getMF().getRegInfo();
  unsigned RegisterUnits = 0;
  unsigned StackOffset = 0;
  for (unsigned I = 0; I != VRegs.size(); ++I) {
    if (VRegs[I].size() != 1)
      return false;
    Register VReg = VRegs[I][0];
    LLT Ty = MRI.getType(VReg);
    if (!isSupportedScalar(Ty))
      return false;
    unsigned Width = Ty.getSizeInBits();
    if (RegisterUnits < 4) {
      Register Phys = argumentRegister(RegisterUnits++, Width);
      MRI.addLiveIn(Phys);
      MIRBuilder.getMBB().addLiveIn(Phys);
      if (Width == 1) {
        Register Byte = MRI.createGenericVirtualRegister(LLT::scalar(8));
        MIRBuilder.buildCopy(Byte, Phys);
        MIRBuilder.buildTrunc(VReg, Byte);
      } else {
        MIRBuilder.buildCopy(VReg, Phys);
      }
    } else {
      buildIncomingStackLoad(MIRBuilder, VReg, abiSize(Ty), StackOffset);
      StackOffset += abiSize(Ty);
    }
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
    LLT Ty = MIRBuilder.getMF().getRegInfo().getType(VReg);
    if (!isSupportedScalar(Ty))
      return false;

    const Function &F = MIRBuilder.getMF().getFunction();
    Register Phys = argumentRegister(0, Width);
    if (Width == 1) {
      MachineRegisterInfo &MRI = MIRBuilder.getMF().getRegInfo();
      Register Byte = MRI.createGenericVirtualRegister(LLT::scalar(8));
      MIRBuilder.buildZExt(Byte, VReg);
      VReg = Byte;
      Width = 8;
      Phys = AVM::B4;
    }
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

bool AVMCallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                CallLoweringInfo &Info) const {
  if (Info.IsVarArg || Info.IsMustTailCall || Info.Callee.isReg())
    return false;
  if (!Info.Callee.isGlobal() && !Info.Callee.isSymbol())
    return false;

  MachineFunction &MF = MIRBuilder.getMF();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const auto *TRI = MF.getSubtarget().getRegisterInfo();

  unsigned RegisterUnits = 0;
  unsigned StackSize = 0;
  for (const ArgInfo &Arg : Info.OrigArgs) {
    if (Arg.Regs.size() != 1)
      return false;
    LLT Ty = MRI.getType(Arg.Regs[0]);
    if (!isSupportedScalar(Ty))
      return false;
    if (RegisterUnits < 4)
      ++RegisterUnits;
    else
      StackSize += abiSize(Ty);
  }

  if (StackSize)
    MIRBuilder.buildInstr(AVM::ADJCALLSTACKDOWN)
        .addImm(StackSize)
        .addImm(0);

  RegisterUnits = 0;
  unsigned StackOffset = 0;
  SmallVector<std::pair<Register, Register>, 4> RegisterArgs;
  for (const ArgInfo &Arg : Info.OrigArgs) {
    Register Value = Arg.Regs[0];
    LLT Ty = MRI.getType(Value);
    unsigned Width = Ty.getSizeInBits();
    if (RegisterUnits < 4) {
      Register Phys = argumentRegister(RegisterUnits++, Width);
      if (Width == 1) {
        Register Byte = MRI.createGenericVirtualRegister(LLT::scalar(8));
        MIRBuilder.buildZExt(Byte, Value);
        Value = Byte;
      }
      RegisterArgs.emplace_back(Phys, Value);
    } else {
      buildOutgoingStackStore(MIRBuilder, Value, abiSize(Ty), StackOffset);
      StackOffset += abiSize(Ty);
    }
  }

  auto Call = MIRBuilder.buildInstrNoInsert(AVM::CALLF).add(Info.Callee);
  for (auto [Phys, Value] : RegisterArgs) {
    MIRBuilder.buildCopy(Phys, Value);
    Call.addUse(Phys, RegState::Implicit);
  }

  Register ReturnPhys;
  if (!Info.OrigRet.Ty->isVoidTy()) {
    if (Info.OrigRet.Regs.size() != 1)
      return false;
    Register Result = Info.OrigRet.Regs[0];
    LLT Ty = MRI.getType(Result);
    if (!isSupportedScalar(Ty))
      return false;
    ReturnPhys = argumentRegister(0, Ty.getSizeInBits());
    Call.addDef(ReturnPhys, RegState::Implicit);
  }
  Call.addRegMask(TRI->getCallPreservedMask(MF, Info.CallConv));
  MIRBuilder.insertInstr(Call);

  if (ReturnPhys)
    MIRBuilder.buildCopy(Info.OrigRet.Regs[0], ReturnPhys);

  if (StackSize)
    MIRBuilder.buildInstr(AVM::ADJCALLSTACKUP)
        .addImm(StackSize)
        .addImm(0);
  return true;
}
