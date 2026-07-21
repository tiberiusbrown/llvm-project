//===-- AVMISelLowering.cpp - AVM DAG lowering --------------------------===//

#include "AVMISelLowering.h"
#include "AVMMachineFunctionInfo.h"
#include "AVMSubtarget.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/IR/IntrinsicsAVM.h"
#include "llvm/IR/RuntimeLibcalls.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

namespace {
constexpr MCPhysReg AVMArgUnits[] = {AVM::R4, AVM::R5, AVM::R6, AVM::R7};
constexpr MCPhysReg AVMArgPairs[] = {AVM::R4R5, AVM::R6R7};

template <typename ArgT> static unsigned getABIUnits(const ArgT &Arg) {
  if (Arg.Flags.isPointer() && Arg.Flags.getPointerAddrSpace() == 1)
    return 2;
  uint64_t Bits = Arg.ArgVT.getSizeInBits().getFixedValue();
  if (Bits <= 16)
    return 1;
  if (Bits <= 32)
    return 2;
  if (Bits <= 64)
    return 4;
  report_fatal_error("unsupported AVM scalar argument size");
}

template <typename ArgT> static unsigned getABIStackBytes(const ArgT &Arg) {
  if (Arg.Flags.isPointer() && Arg.Flags.getPointerAddrSpace() == 1)
    return 3;
  return divideCeil(Arg.ArgVT.getSizeInBits().getFixedValue(), uint64_t(8));
}

static MCPhysReg getAVMArgRegister(unsigned Unit, MVT VT) {
  if (VT == MVT::i32 || VT == MVT::f32) {
    assert(Unit % 2 == 0 && Unit < 4 && "unaligned AVM pair argument");
    return AVMArgPairs[Unit / 2];
  }
  assert((VT == MVT::i8 || VT == MVT::i16) && Unit < 4 &&
         "unsupported legalized AVM argument part");
  return AVMArgUnits[Unit];
}

template <typename ArgT>
static void analyzeAVMArguments(const SmallVectorImpl<ArgT> &Args,
                                CCState &State, bool AllStack = false) {
  unsigned UnitCursor = 0;
  bool RegistersClosed = AllStack;

  for (unsigned I = 0, E = Args.size(); I != E;) {
    unsigned J = I + 1;
    while (J != E && Args[J].OrigArgIndex == Args[I].OrigArgIndex)
      ++J;

    unsigned Units = getABIUnits(Args[I]);
    unsigned UnitAlign = Units == 1 ? 1 : 2;
    unsigned FirstUnit = alignTo(UnitCursor, UnitAlign);
    bool UseRegisters = !RegistersClosed && FirstUnit + Units <= 4 &&
                        (Units != 4 || FirstUnit == 0);

    int64_t StackBase = 0;
    if (UseRegisters) {
      UnitCursor = FirstUnit + Units;
    } else {
      RegistersClosed = true;
      StackBase = State.AllocateStack(getABIStackBytes(Args[I]), Align(1));
    }

    for (unsigned Part = I; Part != J; ++Part) {
      MVT ValVT = Args[Part].VT;
      if (UseRegisters) {
        unsigned Unit = FirstUnit + Args[Part].PartOffset / 2;
        MVT LocVT = ValVT == MVT::i8 ? MVT::i16 : ValVT;
        CCValAssign::LocInfo LocInfo = CCValAssign::Full;
        if (ValVT == MVT::i8)
          LocInfo =
              Args[Part].Flags.isSExt() ? CCValAssign::SExt : CCValAssign::ZExt;
        MCPhysReg Reg = State.AllocateReg(getAVMArgRegister(Unit, LocVT));
        assert(Reg && "AVM argument register was allocated twice");
        State.addLoc(CCValAssign::getReg(Part, ValVT, Reg, LocVT, LocInfo));
        continue;
      }

      MVT LocVT = ValVT;
      CCValAssign::LocInfo LocInfo = CCValAssign::Full;
      if (getABIStackBytes(Args[I]) == 1) {
        LocVT = MVT::i8;
        if (ValVT != MVT::i8)
          LocInfo = CCValAssign::Trunc;
      }
      State.addLoc(CCValAssign::getMem(
          Part, ValVT, StackBase + Args[Part].PartOffset, LocVT, LocInfo));
    }
    I = J;
  }
}

template <typename ArgT>
static bool analyzeAVMReturns(const SmallVectorImpl<ArgT> &Args,
                              CCState &State) {
  unsigned Unit = 0;
  for (unsigned I = 0; I != Args.size(); ++I) {
    MVT ValVT = Args[I].VT;
    uint64_t ABIBits =
        Args[I].Flags.isPointer() && Args[I].Flags.getPointerAddrSpace() == 1
            ? 32
            : Args[I].ArgVT.getSizeInBits().getFixedValue();
    MVT LocVT = ABIBits <= 16 ? MVT::i16 : ValVT;
    if ((ValVT != MVT::i8 && ValVT != MVT::i16 && ValVT != MVT::i32 &&
         ValVT != MVT::f32) ||
        (LocVT != MVT::i16 && LocVT != MVT::i32 && LocVT != MVT::f32) ||
        Unit + (LocVT.getStoreSize() / 2) > 4)
      return false;
    CCValAssign::LocInfo LocInfo =
        ValVT == LocVT ? CCValAssign::Full : CCValAssign::Trunc;
    MCPhysReg Reg = getAVMArgRegister(Unit, LocVT);
    State.addLoc(CCValAssign::getReg(I, ValVT, Reg, LocVT, LocInfo));
    Unit += LocVT.getStoreSize() / 2;
  }
  return true;
}

static SDValue canonicalizeNarrowOutgoing(SDValue Value,
                                          const ISD::OutputArg &Arg,
                                          const SDLoc &DL, SelectionDAG &DAG) {
  if (Value.getValueType() != MVT::i16 ||
      Arg.ArgVT.getSizeInBits().getFixedValue() >= 16)
    return Value;
  if (Arg.Flags.isSExt())
    return DAG.getNode(ISD::SIGN_EXTEND_INREG, DL, MVT::i16, Value,
                       DAG.getValueType(MVT::i8));
  return DAG.getNode(ISD::AND, DL, MVT::i16, Value,
                     DAG.getConstant(0xff, DL, MVT::i16));
}

static AVMCC::CondCode canonicalizeCondCode(ISD::CondCode CC, SDValue &LHS,
                                            SDValue &RHS) {
  switch (CC) {
  case ISD::SETEQ:
    return AVMCC::EQ;
  case ISD::SETNE:
    return AVMCC::NE;
  case ISD::SETULT:
    return AVMCC::ULT;
  case ISD::SETUGE:
    return AVMCC::UGE;
  case ISD::SETLT:
    return AVMCC::SLT;
  case ISD::SETGE:
    return AVMCC::SGE;
  case ISD::SETULE:
    std::swap(LHS, RHS);
    return AVMCC::UGE;
  case ISD::SETUGT:
    std::swap(LHS, RHS);
    return AVMCC::ULT;
  case ISD::SETLE:
    std::swap(LHS, RHS);
    return AVMCC::SGE;
  case ISD::SETGT:
    std::swap(LHS, RHS);
    return AVMCC::SLT;
  default:
    report_fatal_error("unsupported AVM integer condition code");
  }
}

static bool stripByteValue(SDValue Value, SDValue &ByteValue) {
  if (Value.getOpcode() == ISD::AssertZext &&
      cast<VTSDNode>(Value.getOperand(1))->getVT() == MVT::i8) {
    ByteValue = Value.getOperand(0);
    return true;
  }
  if (Value.getOpcode() == ISD::SIGN_EXTEND_INREG &&
      cast<VTSDNode>(Value.getOperand(1))->getVT() == MVT::i8) {
    ByteValue = Value.getOperand(0);
    return true;
  }

  if (Value.getOpcode() == ISD::AND) {
    if (const auto *Mask = dyn_cast<ConstantSDNode>(Value.getOperand(1));
        Mask && Mask->getZExtValue() == 0xff) {
      ByteValue = Value.getOperand(0);
      return true;
    }
  }

  if (const auto *Load = dyn_cast<LoadSDNode>(Value);
      Load && Load->getMemoryVT() == MVT::i8) {
    ByteValue = Value;
    return true;
  }

  return false;
}

static std::pair<SDValue, SDValue> getAVMCompare(SDValue LHS, SDValue RHS,
                                                 ISD::CondCode CC,
                                                 const SDLoc &DL,
                                                 SelectionDAG &DAG) {
  AVMCC::CondCode TargetCond = canonicalizeCondCode(CC, LHS, RHS);
  SDValue TargetCC = DAG.getTargetConstant(TargetCond, DL, MVT::i16);

  if (LHS.getValueType() == MVT::i32) {
    SDValue Glue = DAG.getNode(AVMISD::CMP, DL, MVT::Glue, LHS, RHS);
    return {TargetCC, Glue};
  }

  if (const auto *C = dyn_cast<ConstantSDNode>(RHS); C && C->isZero()) {
    SDValue ByteValue;
    SDValue Glue;
    if (stripByteValue(LHS, ByteValue)) {
      Glue = DAG.getNode(AVMISD::TST8, DL, MVT::Glue, ByteValue);
    } else {
      bool HighByteKnownZero = false;
      if (TargetCond == AVMCC::EQ || TargetCond == AVMCC::NE) {
        KnownBits Known = DAG.computeKnownBits(LHS);
        APInt HighMask = APInt::getHighBitsSet(16, 8);
        HighByteKnownZero = (Known.Zero & HighMask) == HighMask;
      }
      Glue = DAG.getNode(HighByteKnownZero ? AVMISD::TST8 : AVMISD::TST16, DL,
                         MVT::Glue, LHS);
    }
    return {TargetCC, Glue};
  }

  if (const auto *C = dyn_cast<ConstantSDNode>(RHS);
      C && isInt<8>(C->getSExtValue())) {
    SDValue Imm = DAG.getSignedTargetConstant(C->getSExtValue(), DL, MVT::i16);
    SDValue Glue = DAG.getNode(AVMISD::CMPI, DL, MVT::Glue, LHS, Imm);
    return {TargetCC, Glue};
  }

  SDValue Glue = DAG.getNode(AVMISD::CMP, DL, MVT::Glue, LHS, RHS);
  return {TargetCC, Glue};
}

static bool getAVMDirectFloatCompare(SDValue LHS, SDValue RHS, ISD::CondCode CC,
                                     const SDLoc &DL, SelectionDAG &DAG,
                                     SDValue &TargetCC, SDValue &Glue) {
  int16_t Value;
  ISD::CondCode IntCC;

  switch (CC) {
  case ISD::SETOEQ:
  case ISD::SETEQ:
    Value = 0;
    IntCC = ISD::SETEQ;
    break;
  case ISD::SETOGT:
  case ISD::SETGT:
    Value = 1;
    IntCC = ISD::SETEQ;
    break;
  case ISD::SETOGE:
  case ISD::SETGE:
    Value = 2;
    IntCC = ISD::SETULT;
    break;
  case ISD::SETOLT:
  case ISD::SETLT:
    Value = -1;
    IntCC = ISD::SETEQ;
    break;
  case ISD::SETOLE:
  case ISD::SETLE:
    Value = 0;
    IntCC = ISD::SETLE;
    break;
  case ISD::SETO:
    Value = 2;
    IntCC = ISD::SETNE;
    break;
  case ISD::SETUO:
    Value = 2;
    IntCC = ISD::SETEQ;
    break;
  case ISD::SETUGT:
    Value = 0;
    IntCC = ISD::SETGT;
    break;
  case ISD::SETUGE:
    Value = -1;
    IntCC = ISD::SETNE;
    break;
  case ISD::SETULT:
    Value = 2;
    IntCC = ISD::SETUGE;
    break;
  case ISD::SETULE:
    Value = 1;
    IntCC = ISD::SETNE;
    break;
  case ISD::SETUNE:
  case ISD::SETNE:
    Value = 0;
    IntCC = ISD::SETNE;
    break;
  default:
    return false;
  }

  SDValue Result = DAG.getNode(AVMISD::FCMP, DL, MVT::i16, LHS, RHS);
  std::tie(TargetCC, Glue) = getAVMCompare(
      Result, DAG.getConstant(static_cast<uint16_t>(Value), DL, MVT::i16),
      IntCC, DL, DAG);
  return true;
}

static SDValue getAVMCSet(SDValue LHS, SDValue RHS, ISD::CondCode CC,
                          const SDLoc &DL, SelectionDAG &DAG) {
  auto [TargetCC, Glue] = getAVMCompare(LHS, RHS, CC, DL, DAG);
  return DAG.getNode(AVMISD::CSET, DL, MVT::i16, TargetCC, Glue);
}

static SDValue getAVMFloatSetCC(SDValue LHS, SDValue RHS, ISD::CondCode CC,
                                const SDLoc &DL, SelectionDAG &DAG) {
  SDValue Result = DAG.getNode(AVMISD::FCMP, DL, MVT::i16, LHS, RHS);
  auto Compare = [&](int16_t Value, ISD::CondCode IntCC) {
    return getAVMCSet(
        Result, DAG.getConstant(static_cast<uint16_t>(Value), DL, MVT::i16),
        IntCC, DL, DAG);
  };
  auto Equal = [&](int16_t Value) { return Compare(Value, ISD::SETEQ); };
  auto NotEqual = [&](int16_t Value) { return Compare(Value, ISD::SETNE); };
  auto And = [&](SDValue A, SDValue B) {
    return DAG.getNode(ISD::AND, DL, MVT::i16, A, B);
  };
  auto Or = [&](SDValue A, SDValue B) {
    return DAG.getNode(ISD::OR, DL, MVT::i16, A, B);
  };

  switch (CC) {
  case ISD::SETOEQ:
  case ISD::SETEQ:
    return Equal(0);
  case ISD::SETOGT:
  case ISD::SETGT:
    return Equal(1);
  case ISD::SETOGE:
  case ISD::SETGE:
    return Compare(2, ISD::SETULT);
  case ISD::SETOLT:
  case ISD::SETLT:
    return Equal(-1);
  case ISD::SETOLE:
  case ISD::SETLE:
    return Compare(0, ISD::SETLE);
  case ISD::SETONE:
    return And(NotEqual(0), NotEqual(2));
  case ISD::SETO:
    return NotEqual(2);
  case ISD::SETUO:
    return Equal(2);
  case ISD::SETUEQ:
    return Or(Equal(0), Equal(2));
  case ISD::SETUGT:
    return Or(Equal(1), Equal(2));
  case ISD::SETUGE:
    return NotEqual(-1);
  case ISD::SETULT:
    return Or(Equal(-1), Equal(2));
  case ISD::SETULE:
    return NotEqual(1);
  case ISD::SETUNE:
  case ISD::SETNE:
    return NotEqual(0);
  case ISD::SETFALSE:
  case ISD::SETFALSE2:
    return DAG.getConstant(0, DL, MVT::i16);
  case ISD::SETTRUE:
  case ISD::SETTRUE2:
    return DAG.getConstant(1, DL, MVT::i16);
  default:
    report_fatal_error("unsupported AVM floating condition code");
  }
}
} // namespace

AVMTargetLowering::AVMTargetLowering(const TargetMachine &TM,
                                     const AVMSubtarget &STI)
    : TargetLowering(TM, STI) {
  IsStrictFPEnabled = true;
  addRegisterClass(MVT::i16, &AVM::GPR16RegClass);
  addRegisterClass(MVT::i32, &AVM::GPR32RegClass);
  addRegisterClass(MVT::f32, &AVM::GPR32RegClass);

  setStackPointerRegisterToSaveRestore(AVM::SP);
  setBooleanContents(ZeroOrOneBooleanContent);
  setMinFunctionAlignment(Align(1));
  setPrefFunctionAlignment(Align(1));

  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, Custom);
  setOperationAction(ISD::BR_CC, MVT::i16, Custom);
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::BR_CC, MVT::f32, Custom);
  setOperationAction(ISD::GlobalAddress, MVT::i16, Custom);
  setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::i16, Custom);
  setOperationAction(ISD::SETCC, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::f32, Custom);
  if (TM.getOptLevel() == CodeGenOptLevel::None) {
    setOperationAction(ISD::SELECT, MVT::i16, Custom);
    setOperationAction(ISD::SELECT, MVT::i32, Custom);
    setOperationAction(ISD::SELECT, MVT::f32, Custom);
    setOperationAction(ISD::SELECT_CC, MVT::i16, Expand);
    setOperationAction(ISD::SELECT_CC, MVT::i32, Expand);
    setOperationAction(ISD::SELECT_CC, MVT::f32, Expand);
  } else {
    setOperationAction(ISD::SELECT, MVT::i16, Custom);
    setOperationAction(ISD::SELECT, MVT::i32, Custom);
    setOperationAction(ISD::SELECT, MVT::f32, Custom);
    setOperationAction(ISD::SELECT_CC, MVT::i16, Custom);
    setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
    setOperationAction(ISD::SELECT_CC, MVT::f32, Custom);
  }
  for (unsigned Opcode :
       {ISD::ADD, ISD::SUB, ISD::AND, ISD::OR, ISD::XOR, ISD::BSWAP})
    setOperationAction(Opcode, MVT::i32, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i32, Legal);
  for (unsigned Opcode : {ISD::MUL, ISD::UDIV, ISD::UREM, ISD::SDIV, ISD::SREM})
    setOperationAction(Opcode, MVT::i32, LibCall);
  for (unsigned Opcode : {ISD::SHL, ISD::SRL, ISD::SRA})
    setOperationAction(Opcode, MVT::i32, Custom);
  for (MVT VT : {MVT::i16, MVT::i32}) {
    setOperationAction(ISD::ROTL, VT, Expand);
    setOperationAction(ISD::ROTR, VT, Expand);
  }
  for (unsigned Opcode :
       {ISD::MULHU, ISD::MULHS, ISD::UMUL_LOHI, ISD::SMUL_LOHI, ISD::SHL_PARTS,
        ISD::SRL_PARTS, ISD::SRA_PARTS})
    setOperationAction(Opcode, MVT::i32, Expand);
  for (unsigned Opcode : {ISD::MUL, ISD::UDIV, ISD::UREM, ISD::SDIV, ISD::SREM,
                          ISD::SHL, ISD::SRL, ISD::SRA})
    setOperationAction(Opcode, MVT::i64, LibCall);

  setLibcallImpl(RTLIB::MUL_I32, RTLIB::impl___avm_mulsi3);
  setLibcallImpl(RTLIB::UDIV_I32, RTLIB::impl___avm_udivsi3);
  setLibcallImpl(RTLIB::UREM_I32, RTLIB::impl___avm_umodsi3);
  setLibcallImpl(RTLIB::SDIV_I32, RTLIB::impl___avm_divsi3);
  setLibcallImpl(RTLIB::SREM_I32, RTLIB::impl___avm_modsi3);
  setLibcallImpl(RTLIB::SHL_I32, RTLIB::impl___avm_ashlsi3);
  setLibcallImpl(RTLIB::SRL_I32, RTLIB::impl___avm_lshrsi3);
  setLibcallImpl(RTLIB::SRA_I32, RTLIB::impl___avm_ashrsi3);
  setLibcallImpl(RTLIB::MUL_I64, RTLIB::impl___avm_muldi3);
  setLibcallImpl(RTLIB::UDIV_I64, RTLIB::impl___avm_udivdi3);
  setLibcallImpl(RTLIB::UREM_I64, RTLIB::impl___avm_umoddi3);
  setLibcallImpl(RTLIB::SDIV_I64, RTLIB::impl___avm_divdi3);
  setLibcallImpl(RTLIB::SREM_I64, RTLIB::impl___avm_moddi3);
  setLibcallImpl(RTLIB::SHL_I64, RTLIB::impl___avm_ashldi3);
  setLibcallImpl(RTLIB::SRL_I64, RTLIB::impl___avm_lshrdi3);
  setLibcallImpl(RTLIB::SRA_I64, RTLIB::impl___avm_ashrdi3);
  setLibcallImpl(RTLIB::ADD_F32, RTLIB::impl___addsf3);
  setLibcallImpl(RTLIB::SUB_F32, RTLIB::impl___subsf3);
  setLibcallImpl(RTLIB::MUL_F32, RTLIB::impl___mulsf3);
  setLibcallImpl(RTLIB::DIV_F32, RTLIB::impl___divsf3);
  setLibcallImpl(RTLIB::SQRT_F32, RTLIB::impl_sqrtf);
  setLibcallImpl(RTLIB::FPTOSINT_F32_I32, RTLIB::impl___fixsfsi);
  setLibcallImpl(RTLIB::FPTOUINT_F32_I32, RTLIB::impl___fixunssfsi);
  setLibcallImpl(RTLIB::SINTTOFP_I32_F32, RTLIB::impl___floatsisf);
  setLibcallImpl(RTLIB::UINTTOFP_I32_F32, RTLIB::impl___floatunsisf);
  setLibcallImpl(RTLIB::MEMCPY, RTLIB::impl_memcpy);
  setLibcallImpl(RTLIB::MEMSET, RTLIB::impl_memset);
  setLibcallImpl(RTLIB::MEMMOVE, RTLIB::impl_memmove);
  setLibcallImpl(RTLIB::ATOMIC_LOAD, RTLIB::impl___atomic_load);
  setLibcallImpl(RTLIB::ATOMIC_LOAD_8, RTLIB::impl___atomic_load_8);
  setLibcallImpl(RTLIB::ATOMIC_STORE, RTLIB::impl___atomic_store);
  setLibcallImpl(RTLIB::ATOMIC_STORE_8, RTLIB::impl___atomic_store_8);
  setLibcallImpl(RTLIB::ATOMIC_EXCHANGE, RTLIB::impl___atomic_exchange);
  setLibcallImpl(RTLIB::ATOMIC_EXCHANGE_8, RTLIB::impl___atomic_exchange_8);
  setLibcallImpl(RTLIB::ATOMIC_COMPARE_EXCHANGE,
                 RTLIB::impl___atomic_compare_exchange);
  setLibcallImpl(RTLIB::ATOMIC_COMPARE_EXCHANGE_8,
                 RTLIB::impl___atomic_compare_exchange_8);
  setLibcallImpl(RTLIB::ATOMIC_FETCH_ADD_8, RTLIB::impl___atomic_fetch_add_8);
  setLibcallImpl(RTLIB::ATOMIC_FETCH_SUB_8, RTLIB::impl___atomic_fetch_sub_8);
  setLibcallImpl(RTLIB::ATOMIC_FETCH_AND_8, RTLIB::impl___atomic_fetch_and_8);
  setLibcallImpl(RTLIB::ATOMIC_FETCH_OR_8, RTLIB::impl___atomic_fetch_or_8);
  setLibcallImpl(RTLIB::ATOMIC_FETCH_XOR_8, RTLIB::impl___atomic_fetch_xor_8);
  setLibcallImpl(RTLIB::ATOMIC_FETCH_NAND_8, RTLIB::impl___atomic_fetch_nand_8);

  for (unsigned Opcode :
       {ISD::FADD, ISD::FSUB, ISD::FMUL, ISD::FDIV, ISD::FSQRT, ISD::FNEG,
        ISD::FABS, ISD::FMINNUM, ISD::FMAXNUM})
    setOperationAction(Opcode, MVT::f32, Legal);
  for (unsigned Opcode :
       {ISD::FSIN, ISD::FCOS, ISD::FATAN2, ISD::FTAN, ISD::FEXP, ISD::FLOG,
        ISD::FLOG2, ISD::FLOG10, ISD::FPOW, ISD::FREM})
    setOperationAction(Opcode, MVT::f32, Custom);
  for (unsigned Opcode : {ISD::STRICT_FADD, ISD::STRICT_FSUB, ISD::STRICT_FMUL,
                          ISD::STRICT_FDIV, ISD::STRICT_FSQRT})
    setOperationAction(Opcode, MVT::f32, Expand);
  for (MVT VT : {MVT::i16, MVT::i32}) {
    setOperationAction(ISD::SINT_TO_FP, VT, Legal);
    setOperationAction(ISD::UINT_TO_FP, VT, Legal);
    setOperationAction(ISD::FP_TO_SINT, VT, Legal);
    setOperationAction(ISD::FP_TO_UINT, VT, Legal);
    setOperationAction(ISD::STRICT_FP_TO_SINT, VT, Expand);
    setOperationAction(ISD::STRICT_FP_TO_UINT, VT, Expand);
  }
  setOperationAction(ISD::STRICT_SINT_TO_FP, MVT::f32, Expand);
  setOperationAction(ISD::STRICT_UINT_TO_FP, MVT::f32, Expand);
  setOperationAction(ISD::IS_FPCLASS, MVT::f32, Custom);
  setOperationAction(ISD::ADDRSPACECAST, MVT::i16, Custom);
  setOperationAction(ISD::ADDRSPACECAST, MVT::i32, Custom);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i16, Custom);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::VAARG, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Custom);
  setOperationAction(ISD::INTRINSIC_VOID, MVT::i8, Custom);
  setMaxAtomicSizeInBitsSupported(32);

  setLoadExtAction({ISD::EXTLOAD, ISD::ZEXTLOAD, ISD::SEXTLOAD}, MVT::i16,
                   MVT::i8, Legal);
  setLoadExtAction({ISD::EXTLOAD, ISD::ZEXTLOAD}, MVT::i32, MVT::i24, Legal);
  setTruncStoreAction(MVT::i16, MVT::i8, Legal);
  setTruncStoreAction(MVT::i32, MVT::i24, Legal);
  setIndexedLoadAction(ISD::POST_INC, MVT::i8, Legal);
  setIndexedLoadAction(ISD::POST_INC, MVT::i16, Legal);
  setIndexedLoadAction(ISD::POST_INC, MVT::i32, Legal);
  setIndexedStoreAction(ISD::POST_INC, MVT::i8, Legal);
  setIndexedStoreAction(ISD::POST_INC, MVT::i16, Legal);

  computeRegisterProperties(STI.getRegisterInfo());

  MaxStoresPerMemset = MaxStoresPerMemcpy = MaxStoresPerMemmove = 8;
  MaxStoresPerMemsetOptSize = MaxStoresPerMemcpyOptSize =
      MaxStoresPerMemmoveOptSize = 4;
}

bool AVMTargetLowering::isFPImmLegal(const APFloat &, EVT VT, bool) const {
  return VT == MVT::f32;
}

static TargetLowering::AtomicExpansionKind getAVMAtomicExpansionKind(Type *Ty) {
  return Ty->getPrimitiveSizeInBits().getFixedValue() <= 32
             ? TargetLowering::AtomicExpansionKind::NotAtomic
             : TargetLowering::AtomicExpansionKind::None;
}

TargetLowering::AtomicExpansionKind
AVMTargetLowering::shouldExpandAtomicLoadInIR(LoadInst *LI) const {
  return getAVMAtomicExpansionKind(LI->getType());
}

TargetLowering::AtomicExpansionKind
AVMTargetLowering::shouldExpandAtomicStoreInIR(StoreInst *SI) const {
  return getAVMAtomicExpansionKind(SI->getValueOperand()->getType());
}

TargetLowering::AtomicExpansionKind
AVMTargetLowering::shouldExpandAtomicCmpXchgInIR(AtomicCmpXchgInst *CI) const {
  return getAVMAtomicExpansionKind(CI->getCompareOperand()->getType());
}

TargetLowering::AtomicExpansionKind
AVMTargetLowering::shouldExpandAtomicRMWInIR(AtomicRMWInst *AI) const {
  return getAVMAtomicExpansionKind(AI->getValOperand()->getType());
}

MVT AVMTargetLowering::getPointerTy(const DataLayout &DL, uint32_t AS) const {
  if (AS == 1)
    return MVT::i32;
  return TargetLowering::getPointerTy(DL, AS);
}

EVT AVMTargetLowering::getTypeForExtReturn(LLVMContext &, EVT VT,
                                           ISD::NodeType) const {
  return VT;
}

const char *AVMTargetLowering::getTargetNodeName(unsigned Opcode) const {
  if (Opcode == AVMISD::BR_CC)
    return "AVMISD::BR_CC";
  if (Opcode == AVMISD::CALL)
    return "AVMISD::CALL";
  if (Opcode == AVMISD::CMOV)
    return "AVMISD::CMOV";
  if (Opcode == AVMISD::CMP)
    return "AVMISD::CMP";
  if (Opcode == AVMISD::CMPI)
    return "AVMISD::CMPI";
  if (Opcode == AVMISD::CSET)
    return "AVMISD::CSET";
  if (Opcode == AVMISD::FCMP)
    return "AVMISD::FCMP";
  if (Opcode == AVMISD::FCLASS)
    return "AVMISD::FCLASS";
  if (Opcode == AVMISD::LOAD24)
    return "AVMISD::LOAD24";
  if (Opcode == AVMISD::NORMALIZE_PROGPTR)
    return "AVMISD::NORMALIZE_PROGPTR";
  if (Opcode == AVMISD::PROG_WRAPPER)
    return "AVMISD::PROG_WRAPPER";
  if (Opcode == AVMISD::STORE24)
    return "AVMISD::STORE24";
  if (Opcode == AVMISD::TST8)
    return "AVMISD::TST8";
  if (Opcode == AVMISD::TST16)
    return "AVMISD::TST16";
  if (Opcode == AVMISD::WRAPPER)
    return "AVMISD::WRAPPER";
  if (Opcode == AVMISD::SHL32_16)
    return "AVMISD::SHL32_16";
  if (Opcode == AVMISD::SRL32_16)
    return "AVMISD::SRL32_16";
  if (Opcode == AVMISD::SRA32_16)
    return "AVMISD::SRA32_16";
  if (Opcode == AVMISD::RET_GLUE)
    return "AVMISD::RET_GLUE";
  return nullptr;
}

bool AVMTargetLowering::allowsMisalignedMemoryAccesses(EVT VT,
                                                       unsigned AddrSpace,
                                                       Align,
                                                       MachineMemOperand::Flags,
                                                       unsigned *Fast) const {
  if (AddrSpace == 0) {
    if (VT != MVT::i8 && VT != MVT::i16 && VT.getSizeInBits() != 24 &&
        VT != MVT::i32 && VT != MVT::f32)
      return false;
  } else if (AddrSpace == 1) {
    if (VT != MVT::i8 && VT != MVT::i16 && VT.getSizeInBits() != 24 &&
        VT != MVT::i32 && VT != MVT::f32)
      return false;
  } else {
    return false;
  }
  if (Fast)
    *Fast = 1;
  return true;
}

bool AVMTargetLowering::getPostIndexedAddressParts(SDNode *N, SDNode *Op,
                                                   SDValue &Base,
                                                   SDValue &Offset,
                                                   ISD::MemIndexedMode &AM,
                                                   SelectionDAG &DAG) const {
  EVT MemoryVT;
  SDValue Pointer;
  unsigned AddressSpace;
  if (const auto *Load = dyn_cast<LoadSDNode>(N)) {
    MemoryVT = Load->getMemoryVT();
    Pointer = Load->getBasePtr();
    AddressSpace = Load->getAddressSpace();
  } else if (const auto *Store = dyn_cast<StoreSDNode>(N)) {
    MemoryVT = Store->getMemoryVT();
    Pointer = Store->getBasePtr();
    AddressSpace = Store->getAddressSpace();
  } else {
    return false;
  }
  if (Op->getOpcode() != ISD::ADD)
    return false;
  if (AddressSpace == 0 && MemoryVT != MVT::i8 && MemoryVT != MVT::i16)
    return false;
  if (AddressSpace == 1 && MemoryVT != MVT::i8 && MemoryVT != MVT::i16 &&
      MemoryVT.getSizeInBits() != 24 && MemoryVT != MVT::i32 &&
      MemoryVT != MVT::f32)
    return false;
  if (AddressSpace != 0 && AddressSpace != 1)
    return false;
  const auto *Increment = dyn_cast<ConstantSDNode>(Op->getOperand(1));
  int64_t Width = MemoryVT.getStoreSize();
  if (!Increment || Increment->getSExtValue() != Width ||
      Pointer != Op->getOperand(0))
    return false;
  Base = Pointer;
  Offset = DAG.getConstant(Width, SDLoc(N), Pointer.getValueType());
  AM = ISD::POST_INC;
  return true;
}

EVT AVMTargetLowering::getOptimalMemOpType(LLVMContext &, const MemOp &,
                                           const AttributeList &) const {
  // Byte stores preserve memset's value without constructing an i16/i32 splat,
  // and make overlap-safe memmove expansion independent of word alignment.
  return MVT::i8;
}

TargetLowering::ConstraintType
AVMTargetLowering::getConstraintType(StringRef Constraint) const {
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    case 'r':
    case 'c':
    case 'b':
    case 'B':
    case 'p':
    case 'P':
    case 'q':
    case 'Q':
    case 't':
      return C_RegisterClass;
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
      return C_Immediate;
    case 'm':
    case 'o':
      return C_Memory;
    default:
      break;
    }
  }
  return TargetLowering::getConstraintType(Constraint);
}

std::pair<unsigned, const TargetRegisterClass *>
AVMTargetLowering::getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                                                StringRef Constraint,
                                                MVT VT) const {
  unsigned FixedReg = StringSwitch<unsigned>(Constraint)
                          .Case("{r0}", AVM::R0)
                          .Case("{r1}", AVM::R1)
                          .Case("{r2}", AVM::R2)
                          .Case("{r3}", AVM::R3)
                          .Case("{r4}", AVM::R4)
                          .Case("{r5}", AVM::R5)
                          .Case("{r6}", AVM::R6)
                          .Case("{r7}", AVM::R7)
                          .Case("{q0}", AVM::R0R1)
                          .Case("{q1}", AVM::R2R3)
                          .Case("{q2}", AVM::R4R5)
                          .Case("{q3}", AVM::R6R7)
                          .Default(0);
  if (FixedReg)
    return {FixedReg, AVM::GPR32RegClass.contains(FixedReg)
                          ? &AVM::GPR32RegClass
                          : &AVM::GPR16RegClass};

  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    case 'r':
      return {0U, &AVM::GPR16RegClass};
    case 'c':
      return {0U, &AVM::UpperGPR16RegClass};
    case 'b':
      return {0U, &AVM::GPR8RegClass};
    case 'B':
      return {0U, &AVM::UpperGPR8RegClass};
    case 'p':
      return {0U, &AVM::PTR16RegClass};
    case 'P':
      return {0U, &AVM::UpperPTR16RegClass};
    case 'q':
      return {0U, &AVM::GPR32RegClass};
    case 't':
      return {0U, &AVM::ProgPtrGPR32RegClass};
    case 'Q':
      return {0U, &AVM::UpperGPR32RegClass};
    default:
      break;
    }
  }
  return TargetLowering::getRegForInlineAsmConstraint(TRI, Constraint, VT);
}

TargetLowering::ConstraintWeight
AVMTargetLowering::getSingleConstraintMatchWeight(
    AsmOperandInfo &Info, const char *Constraint) const {
  if (*Constraint == 't') {
    auto *Ty = Info.CallOperandVal ? Info.CallOperandVal->getType() : nullptr;
    return Ty && Ty->isPointerTy() &&
                   cast<PointerType>(Ty)->getAddressSpace() == 1
               ? CW_Register
               : CW_Invalid;
  }
  if (StringRef("IJKLMNO").contains(*Constraint))
    return isa_and_nonnull<ConstantInt>(Info.CallOperandVal) ? CW_Constant
                                                             : CW_Invalid;
  return TargetLowering::getSingleConstraintMatchWeight(Info, Constraint);
}

void AVMTargetLowering::LowerAsmOperandForConstraint(SDValue Op,
                                                     StringRef Constraint,
                                                     std::vector<SDValue> &Ops,
                                                     SelectionDAG &DAG) const {
  if (Constraint.size() != 1)
    return TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops,
                                                        DAG);
  const auto *Constant = dyn_cast<ConstantSDNode>(Op);
  if (!StringRef("IJKLMNO").contains(Constraint[0]) || !Constant)
    return TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops,
                                                        DAG);

  int64_t Signed = Constant->getSExtValue();
  uint64_t Unsigned = Constant->getZExtValue();
  bool Matches = false;
  switch (Constraint[0]) {
  case 'I':
    Matches = isInt<8>(Signed);
    break;
  case 'J':
    Matches = isUInt<8>(Unsigned);
    break;
  case 'K':
  case 'N':
    Matches = isUInt<4>(Unsigned);
    break;
  case 'L':
    Matches = isInt<16>(Signed);
    break;
  case 'M':
    Matches = isUInt<16>(Unsigned);
    break;
  case 'O':
    Matches = Unsigned == 0;
    break;
  default:
    llvm_unreachable("invalid AVM immediate constraint");
  }
  if (!Matches)
    return;
  if (Constraint[0] == 'I' || Constraint[0] == 'L')
    Ops.push_back(
        DAG.getSignedTargetConstant(Signed, SDLoc(Op), Op.getValueType()));
  else
    Ops.push_back(DAG.getTargetConstant(Unsigned, SDLoc(Op), MVT::i32));
}

SDValue AVMTargetLowering::LowerGlobalAddress(SDValue Op,
                                              SelectionDAG &DAG) const {
  const auto *GA = cast<GlobalAddressSDNode>(Op);
  SDLoc DL(Op);
  if (GA->getGlobal()->getAddressSpace() == 1) {
    SDValue Target = DAG.getTargetGlobalAddress(
        GA->getGlobal(), DL, MVT::i32, GA->getOffset(), GA->getTargetFlags());
    return DAG.getNode(AVMISD::PROG_WRAPPER, DL, MVT::i32, Target);
  }
  SDValue Target = DAG.getTargetGlobalAddress(
      GA->getGlobal(), DL, MVT::i16, GA->getOffset(), GA->getTargetFlags());
  return DAG.getNode(AVMISD::WRAPPER, DL, MVT::i16, Target);
}

SDValue AVMTargetLowering::LowerBRCC(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  if (Op.getOperand(2).getValueType() == MVT::f32) {
    SDValue TargetCC;
    SDValue Glue;
    if (getAVMDirectFloatCompare(Op.getOperand(2), Op.getOperand(3), CC, DL,
                                 DAG, TargetCC, Glue))
      return DAG.getNode(AVMISD::BR_CC, DL, MVT::Other, Op.getOperand(0),
                         Op.getOperand(4), TargetCC, Glue);
    SDValue Bool =
        getAVMFloatSetCC(Op.getOperand(2), Op.getOperand(3), CC, DL, DAG);
    std::tie(TargetCC, Glue) = getAVMCompare(
        Bool, DAG.getConstant(0, DL, MVT::i16), ISD::SETNE, DL, DAG);
    return DAG.getNode(AVMISD::BR_CC, DL, MVT::Other, Op.getOperand(0),
                       Op.getOperand(4), TargetCC, Glue);
  }
  auto [TargetCC, Glue] =
      getAVMCompare(Op.getOperand(2), Op.getOperand(3), CC, DL, DAG);
  return DAG.getNode(AVMISD::BR_CC, DL, MVT::Other, Op.getOperand(0),
                     Op.getOperand(4), TargetCC, Glue);
}

SDValue AVMTargetLowering::LowerSetCC(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
  if (Op.getOperand(0).getValueType() == MVT::f32)
    return getAVMFloatSetCC(Op.getOperand(0), Op.getOperand(1), CC, DL, DAG);
  auto [TargetCC, Glue] =
      getAVMCompare(Op.getOperand(0), Op.getOperand(1), CC, DL, DAG);
  return DAG.getNode(AVMISD::CSET, DL, Op.getValueType(), TargetCC, Glue);
}

SDValue AVMTargetLowering::LowerSelectCC(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
  SDValue CompareLHS = Op.getOperand(0);
  SDValue CompareRHS = Op.getOperand(1);
  if (CompareLHS.getValueType() == MVT::f32) {
    SDValue TargetCC;
    SDValue Glue;
    if (getAVMDirectFloatCompare(CompareLHS, CompareRHS, CC, DL, DAG, TargetCC,
                                 Glue))
      return DAG.getNode(AVMISD::CMOV, DL, Op.getValueType(), Op.getOperand(2),
                         Op.getOperand(3), TargetCC, Glue);
    CompareLHS = getAVMFloatSetCC(CompareLHS, CompareRHS, CC, DL, DAG);
    CompareRHS = DAG.getConstant(0, DL, MVT::i16);
    CC = ISD::SETNE;
  }
  auto [TargetCC, Glue] = getAVMCompare(CompareLHS, CompareRHS, CC, DL, DAG);
  return DAG.getNode(AVMISD::CMOV, DL, Op.getValueType(), Op.getOperand(2),
                     Op.getOperand(3), TargetCC, Glue);
}

SDValue AVMTargetLowering::LowerSelect(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Cond = Op.getOperand(0);
  SDValue TargetCC;
  SDValue Glue;
  if (Cond.getOpcode() == ISD::SETCC) {
    ISD::CondCode CC = cast<CondCodeSDNode>(Cond.getOperand(2))->get();
    if (Cond.getOperand(0).getValueType() == MVT::f32) {
      if (!getAVMDirectFloatCompare(Cond.getOperand(0), Cond.getOperand(1), CC,
                                    DL, DAG, TargetCC, Glue)) {
        SDValue Bool = getAVMFloatSetCC(Cond.getOperand(0), Cond.getOperand(1),
                                        CC, DL, DAG);
        std::tie(TargetCC, Glue) = getAVMCompare(
            Bool, DAG.getConstant(0, DL, MVT::i16), ISD::SETNE, DL, DAG);
      }
    } else {
      std::tie(TargetCC, Glue) =
          getAVMCompare(Cond.getOperand(0), Cond.getOperand(1), CC, DL, DAG);
    }
  } else {
    std::tie(TargetCC, Glue) = getAVMCompare(
        Cond, DAG.getConstant(0, DL, Cond.getValueType()), ISD::SETNE, DL, DAG);
  }
  return DAG.getNode(AVMISD::CMOV, DL, Op.getValueType(), Op.getOperand(1),
                     Op.getOperand(2), TargetCC, Glue);
}

SDValue AVMTargetLowering::LowerISFPClass(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Class = DAG.getNode(AVMISD::FCLASS, DL, MVT::i16, Op.getOperand(0));
  uint64_t Mask = cast<ConstantSDNode>(Op.getOperand(1))->getZExtValue();
  SDValue Selected = DAG.getNode(ISD::AND, DL, MVT::i16, Class,
                                 DAG.getConstant(Mask, DL, MVT::i16));
  return getAVMCSet(Selected, DAG.getConstant(0, DL, MVT::i16), ISD::SETNE, DL,
                    DAG);
}

SDValue AVMTargetLowering::LowerVASTART(SDValue Op, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  const auto *Info = MF.getInfo<AVMMachineFunctionInfo>();
  SDValue FrameIndex =
      DAG.getFrameIndex(Info->getVarArgsFrameIndex(), MVT::i16);
  const Value *Source = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Op.getOperand(0), SDLoc(Op), FrameIndex, Op.getOperand(1),
                      MachinePointerInfo(Source), Align(1));
}

SDValue AVMTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::INTRINSIC_VOID: {
    const auto *ID = cast<ConstantSDNode>(Op.getOperand(1));
    if (ID->getZExtValue() != Intrinsic::avm_debug_putc)
      report_fatal_error("unexpected AVM intrinsic with an i8 operand");
    SDLoc DL(Op);
    SDValue Value =
        DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i16, Op.getOperand(2));
    return DAG.getNode(ISD::INTRINSIC_VOID, DL, MVT::Other, Op.getOperand(0),
                       Op.getOperand(1), Value);
  }
  case ISD::BR_CC:
    return LowerBRCC(Op, DAG);
  case ISD::BRCOND: {
    SDLoc DL(Op);
    SDValue Cond = Op.getOperand(1);
    auto [TargetCC, Glue] = getAVMCompare(
        Cond, DAG.getConstant(0, DL, Cond.getValueType()), ISD::SETNE, DL, DAG);
    return DAG.getNode(AVMISD::BR_CC, DL, MVT::Other, Op.getOperand(0),
                       Op.getOperand(2), TargetCC, Glue);
  }
  case ISD::GlobalAddress:
    return LowerGlobalAddress(Op, DAG);
  case ISD::IS_FPCLASS:
    return LowerISFPClass(Op, DAG);
  case ISD::VASTART:
    return LowerVASTART(Op, DAG);
  case ISD::ATOMIC_FENCE:
    return Op.getOperand(0);
  case ISD::DYNAMIC_STACKALLOC:
    report_fatal_error("dynamic AVM stack allocation is unsupported");
  case ISD::ADDRSPACECAST:
    report_fatal_error(
        "AVM does not support casts between address spaces 0 and 1");
  case ISD::SHL:
  case ISD::SRL:
  case ISD::SRA: {
    if (const auto *Count = dyn_cast<ConstantSDNode>(Op.getOperand(1));
        Count && Count->getZExtValue() == 16) {
      unsigned Opcode = Op.getOpcode() == ISD::SHL   ? AVMISD::SHL32_16
                        : Op.getOpcode() == ISD::SRL ? AVMISD::SRL32_16
                                                     : AVMISD::SRA32_16;
      return DAG.getNode(Opcode, SDLoc(Op), MVT::i32, Op.getOperand(0));
    }
    RTLIB::Libcall LC = Op.getOpcode() == ISD::SHL   ? RTLIB::SHL_I32
                        : Op.getOpcode() == ISD::SRL ? RTLIB::SRL_I32
                                                     : RTLIB::SRA_I32;
    MakeLibCallOptions CallOptions;
    SDValue Ops[] = {Op.getOperand(0), Op.getOperand(1)};
    return makeLibCall(DAG, LC, MVT::i32, Ops, CallOptions, SDLoc(Op)).first;
  }
  case ISD::SELECT:
    return LowerSelect(Op, DAG);
  case ISD::SELECT_CC:
    return LowerSelectCC(Op, DAG);
  case ISD::SETCC:
    return LowerSetCC(Op, DAG);
  case ISD::FSIN:
  case ISD::FCOS:
  case ISD::FATAN2:
  case ISD::FTAN:
  case ISD::FEXP:
  case ISD::FLOG:
  case ISD::FLOG2:
  case ISD::FLOG10:
  case ISD::FPOW:
  case ISD::FREM: {
    Intrinsic::ID ID;
    switch (Op.getOpcode()) {
    case ISD::FSIN:
      ID = Intrinsic::avm_sinf;
      break;
    case ISD::FCOS:
      ID = Intrinsic::avm_cosf;
      break;
    case ISD::FATAN2:
      ID = Intrinsic::avm_atan2f;
      break;
    case ISD::FTAN:
      ID = Intrinsic::avm_tanf;
      break;
    case ISD::FEXP:
      ID = Intrinsic::avm_expf;
      break;
    case ISD::FLOG:
      ID = Intrinsic::avm_logf;
      break;
    case ISD::FLOG2:
      ID = Intrinsic::avm_log2f;
      break;
    case ISD::FLOG10:
      ID = Intrinsic::avm_log10f;
      break;
    case ISD::FPOW:
      ID = Intrinsic::avm_powf;
      break;
    case ISD::FREM:
      ID = Intrinsic::avm_fmodf;
      break;
    default:
      llvm_unreachable("unexpected AVM math service");
    }
    SmallVector<SDValue, 3> Ops = {
        DAG.getTargetConstant(ID, SDLoc(Op), MVT::i16)};
    Ops.append(Op->op_begin(), Op->op_end());
    return DAG.getNode(ISD::INTRINSIC_WO_CHAIN, SDLoc(Op), MVT::f32, Ops);
  }
  default:
    break;
  }
  llvm_unreachable("unexpected AVM custom-lowered operation");
}

SDValue AVMTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  if (CallConv != CallingConv::C)
    report_fatal_error("unsupported AVM calling convention");

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  SmallVector<CCValAssign, 8> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  analyzeAVMArguments(Ins, CCInfo, IsVarArg);

  MachineRegisterInfo &MRI = MF.getRegInfo();
  SmallVector<SDValue, 4> LoadChains;
  for (const CCValAssign &VA : ArgLocs) {
    unsigned I = VA.getValNo();
    if (VA.isRegLoc()) {
      const TargetRegisterClass *RC =
          VA.getLocVT() == MVT::i32 || VA.getLocVT() == MVT::f32
              ? &AVM::GPR32RegClass
              : &AVM::GPR16RegClass;
      Register VReg = MRI.createVirtualRegister(RC);
      MRI.addLiveIn(VA.getLocReg(), VReg);
      SDValue Value = DAG.getCopyFromReg(Chain, DL, VReg, VA.getLocVT());
      if (VA.getLocVT() == MVT::i16 &&
          Ins[I].ArgVT.getSizeInBits().getFixedValue() < 16) {
        unsigned AssertOp =
            Ins[I].Flags.isSExt() ? ISD::AssertSext : ISD::AssertZext;
        Value = DAG.getNode(AssertOp, DL, MVT::i16, Value,
                            DAG.getValueType(MVT::i8));
      }
      InVals.push_back(Value);
      continue;
    }

    assert(VA.isMemLoc() && "invalid AVM formal-argument location");
    bool IsProgramPointer =
        Ins[I].Flags.isPointer() && Ins[I].Flags.getPointerAddrSpace() == 1;
    bool IsThreeByteValue = Ins[I].ArgVT.getSizeInBits().getFixedValue() == 24;
    unsigned Size =
        IsProgramPointer || IsThreeByteValue ? 3 : VA.getLocVT().getStoreSize();
    int FI = MFI.CreateFixedObject(Size, VA.getLocMemOffset() + 3, true);
    SDValue FIN = DAG.getFrameIndex(FI, MVT::i16);
    MachinePointerInfo PtrInfo = MachinePointerInfo::getFixedStack(MF, FI);
    SDValue Load;
    if (IsProgramPointer || IsThreeByteValue) {
      Load = DAG.getNode(AVMISD::LOAD24, DL,
                         DAG.getVTList(MVT::i32, MVT::Other), Chain, FIN);
    } else if (VA.getLocVT() == VA.getValVT()) {
      Load = DAG.getLoad(VA.getValVT(), DL, Chain, FIN, PtrInfo, Align(1));
    } else {
      ISD::LoadExtType Ext =
          Ins[I].Flags.isSExt() ? ISD::SEXTLOAD : ISD::ZEXTLOAD;
      Load = DAG.getExtLoad(Ext, DL, VA.getValVT(), Chain, FIN, PtrInfo,
                            VA.getLocVT(), Align(1));
    }
    InVals.push_back(Load);
    LoadChains.push_back(Load.getValue(1));
  }
  if (!LoadChains.empty()) {
    LoadChains.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, LoadChains);
  }

  AVMMachineFunctionInfo *Info = MF.getInfo<AVMMachineFunctionInfo>();
  for (unsigned I = 0; I != Ins.size(); ++I) {
    if (!Ins[I].Flags.isSRet())
      continue;
    Register Reg = Info->getSRetReturnReg();
    if (!Reg) {
      Reg = MRI.createVirtualRegister(&AVM::GPR16RegClass);
      Info->setSRetReturnReg(Reg);
    }
    SDValue Copy = DAG.getCopyToReg(DAG.getEntryNode(), DL, Reg, InVals[I]);
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, Copy, Chain);
  }
  if (IsVarArg) {
    int FI = MFI.CreateFixedObject(1, CCInfo.getStackSize() + 3, true);
    Info->setVarArgsFrameIndex(FI);
  }
  return Chain;
}

SDValue AVMTargetLowering::LowerCall(CallLoweringInfo &CLI,
                                     SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  const SDLoc &DL = CLI.DL;
  if (CLI.CallConv != CallingConv::C)
    report_fatal_error("unsupported AVM calling convention");
  CLI.IsTailCall = false;

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  SDValue Chain = CLI.Chain;
  SmallVector<SDValue, 8> OutVals(CLI.OutVals.begin(), CLI.OutVals.end());
  for (unsigned I = 0; I != CLI.Outs.size(); ++I) {
    const ISD::ArgFlagsTy &Flags = CLI.Outs[I].Flags;
    if (!Flags.isByVal() || !Flags.getByValSize())
      continue;
    int FI = MFI.CreateStackObject(Flags.getByValSize(), Align(1), false);
    SDValue Copy = DAG.getFrameIndex(FI, MVT::i16);
    SDValue Size = DAG.getConstant(Flags.getByValSize(), DL, MVT::i16);
    Chain = DAG.getMemcpy(Chain, DL, Copy, OutVals[I], Size, Align(1),
                          /*IsVolatile=*/false, /*AlwaysInline=*/false,
                          /*CI=*/nullptr, std::nullopt, MachinePointerInfo(),
                          MachinePointerInfo());
    OutVals[I] = Copy;
  }

  SmallVector<CCValAssign, 8> ArgLocs;
  CCState CCInfo(CLI.CallConv, CLI.IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  analyzeAVMArguments(CLI.Outs, CCInfo, CLI.IsVarArg);

  unsigned NumBytes = CCInfo.getStackSize();
  Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, DL);
  SmallVector<std::pair<MCPhysReg, SDValue>, 4> RegsToPass;
  SmallVector<SDValue, 4> StoreChains;

  for (const CCValAssign &VA : ArgLocs) {
    unsigned I = VA.getValNo();
    SDValue Arg = OutVals[I];
    if (VA.isRegLoc()) {
      Arg = canonicalizeNarrowOutgoing(Arg, CLI.Outs[I], DL, DAG);
      if (CLI.Outs[I].Flags.isPointer() &&
          CLI.Outs[I].Flags.getPointerAddrSpace() == 1)
        Arg = DAG.getNode(AVMISD::NORMALIZE_PROGPTR, DL, MVT::i32, Arg);
      RegsToPass.emplace_back(VA.getLocReg(), Arg);
      continue;
    }

    assert(VA.isMemLoc() && "invalid AVM call-argument location");
    bool IsProgramPointer = CLI.Outs[I].Flags.isPointer() &&
                            CLI.Outs[I].Flags.getPointerAddrSpace() == 1;
    bool IsThreeByteValue =
        CLI.Outs[I].ArgVT.getSizeInBits().getFixedValue() == 24;
    if (VA.getValVT() != MVT::i16 && VA.getValVT() != MVT::i32 &&
        VA.getValVT() != MVT::f32)
      report_fatal_error(
          "stack lowering for this AVM scalar type is not implemented yet");
    SDValue Ptr =
        DAG.getNode(ISD::ADD, DL, MVT::i16, DAG.getRegister(AVM::SP, MVT::i16),
                    DAG.getConstant(VA.getLocMemOffset(), DL, MVT::i16));
    MachinePointerInfo PtrInfo = MachinePointerInfo::getStack(
        DAG.getMachineFunction(), VA.getLocMemOffset());
    SDValue Store;
    if (IsProgramPointer || IsThreeByteValue)
      Store = DAG.getNode(AVMISD::STORE24, DL, MVT::Other, Chain, Arg,
                          DAG.getConstant(VA.getLocMemOffset(), DL, MVT::i16));
    else if (VA.getLocVT() == MVT::i8)
      Store =
          DAG.getTruncStore(Chain, DL, Arg, Ptr, PtrInfo, MVT::i8, Align(1));
    else
      Store = DAG.getStore(Chain, DL, Arg, Ptr, PtrInfo, Align(1));
    StoreChains.push_back(Store);
  }
  if (!StoreChains.empty()) {
    StoreChains.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, StoreChains);
  }

  SDValue Glue;
  for (const auto &[Reg, Value] : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, DL, Reg, Value, Glue);
    Glue = Chain.getValue(1);
  }

  SDValue Callee = CLI.Callee;
  if (const auto *G = dyn_cast<GlobalAddressSDNode>(Callee))
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), DL, MVT::i32);
  else if (const auto *E = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), MVT::i32);
  else {
    if (Callee.getValueType() != MVT::i32)
      Callee = DAG.getZExtOrTrunc(Callee, DL, MVT::i32);
  }

  SmallVector<SDValue, 10> Ops = {Chain, Callee};
  for (const auto &[Reg, Value] : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg, Value.getValueType()));
  const TargetRegisterInfo *TRI =
      DAG.getMachineFunction().getSubtarget().getRegisterInfo();
  const uint32_t *Mask =
      TRI->getCallPreservedMask(DAG.getMachineFunction(), CLI.CallConv);
  assert(Mask && "missing AVM call-preserved mask");
  Ops.push_back(DAG.getRegisterMask(Mask));
  if (Glue)
    Ops.push_back(Glue);

  Chain =
      DAG.getNode(AVMISD::CALL, DL, DAG.getVTList(MVT::Other, MVT::Glue), Ops);
  Glue = Chain.getValue(1);
  Chain = DAG.getCALLSEQ_END(Chain, NumBytes, 0, Glue, DL);
  Glue = Chain.getValue(1);

  SmallVector<CCValAssign, 4> RVLocs;
  CCState RVInfo(CLI.CallConv, CLI.IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  if (!analyzeAVMReturns(CLI.Ins, RVInfo))
    report_fatal_error("unsupported AVM call return type");
  for (const CCValAssign &VA : RVLocs) {
    SDValue Copy =
        DAG.getCopyFromReg(Chain, DL, VA.getLocReg(), VA.getLocVT(), Glue);
    InVals.push_back(Copy);
    Chain = Copy.getValue(1);
    Glue = Copy.getValue(2);
  }
  return Chain;
}

bool AVMTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *) const {
  if (CallConv != CallingConv::C)
    return false;
  SmallVector<CCValAssign, 2> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);
  return analyzeAVMReturns(Outs, CCInfo);
}

SDValue
AVMTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::OutputArg> &Outs,
                               const SmallVectorImpl<SDValue> &OutVals,
                               const SDLoc &DL, SelectionDAG &DAG) const {
  if (CallConv != CallingConv::C)
    report_fatal_error("unsupported AVM return calling convention");

  SmallVector<CCValAssign, 2> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  if (!analyzeAVMReturns(Outs, CCInfo))
    report_fatal_error("unsupported AVM return type");

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  for (unsigned I = 0; I != RVLocs.size(); ++I) {
    const CCValAssign &VA = RVLocs[I];
    assert(VA.isRegLoc() && "AVM returns must use registers");
    SDValue Value = OutVals[VA.getValNo()];
    if (VA.getLocInfo() == CCValAssign::Trunc)
      Value = DAG.getNode(ISD::TRUNCATE, DL, VA.getLocVT(), Value);
    else
      assert(VA.getLocInfo() == CCValAssign::Full &&
             "unsupported AVM return conversion");
    unsigned ValueIndex = VA.getValNo();
    if (Outs[ValueIndex].Flags.isPointer() &&
        Outs[ValueIndex].Flags.getPointerAddrSpace() == 1)
      Value = DAG.getNode(AVMISD::NORMALIZE_PROGPTR, DL, MVT::i32, Value);
    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), Value, Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  MachineFunction &MF = DAG.getMachineFunction();
  if (MF.getFunction().hasStructRetAttr()) {
    const auto *Info = MF.getInfo<AVMMachineFunctionInfo>();
    Register Reg = Info->getSRetReturnReg();
    assert(Reg && "missing AVM sret return register");
    SDValue Value = DAG.getCopyFromReg(Chain, DL, Reg, MVT::i16);
    Chain = DAG.getCopyToReg(Chain, DL, AVM::R4, Value, Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(AVM::R4, MVT::i16));
  }

  RetOps[0] = Chain;
  if (Glue)
    RetOps.push_back(Glue);
  return DAG.getNode(AVMISD::RET_GLUE, DL, MVT::Other, RetOps);
}
