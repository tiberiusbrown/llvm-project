//===-- AVMISelLowering.cpp - AVM DAG lowering --------------------------===//

#include "AVMISelLowering.h"
#include "AVMCostModel.h"
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
#include <algorithm>

using namespace llvm;

static SDValue getI32Word(SDValue Value, unsigned Index, const SDLoc &DL,
                         SelectionDAG &DAG);
static SDValue lowerFPClass(SDValue Value, unsigned Mask, const SDLoc &DL,
                            SelectionDAG &DAG);
static SDValue lowerClassCompare(SDValue LHS, SDValue RHS, ISD::CondCode CC,
                                 const SDLoc &DL, SelectionDAG &DAG);

bool AVMTargetLowering::decomposeMulByConstant(LLVMContext &, EVT VT,
                                               SDValue C) const {
  if (VT != MVT::i16)
    return false;
  const auto *Constant = dyn_cast<ConstantSDNode>(C);
  if (!Constant)
    return false;

  // The generic DAG combiner can turn 2^N +/- 1 into a shift and an add/sub.
  // Count the extra copy needed to retain the original operand, as well as the
  // immediate materialization that a MUL16 would require. A small margin
  // accounts for the extra live value introduced by the decomposition.
  uint64_t Multiplier = Constant->getZExtValue();
  if (Multiplier < 3 || Multiplier == 5 || Multiplier > INT16_MAX)
    return false;
  bool IsAdd = isPowerOf2_64(Multiplier - 1);
  if (!IsAdd && !isPowerOf2_64(Multiplier + 1))
    return false;
  unsigned Shift = Log2_64(IsAdd ? Multiplier - 1 : Multiplier + 1);
  if (Shift >= 16)
    return false;

  using AVM::AVMCostKind;
  unsigned MultiplyCycles =
      AVM::getFixedCycles(Multiplier <= UINT8_MAX ? AVMCostKind::Ldi8Upper
                                                  : AVMCostKind::Ldi16Upper) +
      AVM::getFixedCycles(AVMCostKind::Mul16);
  unsigned ShiftCycles =
      Shift <= 3 ? Shift * AVM::getFixedCycles(AVMCostKind::AddUpper)
                 : AVM::getShiftCycles(AVMCostKind::Lsl16I, Shift);
  unsigned DecomposedCycles =
      AVM::getFixedCycles(AVMCostKind::MovUpper) + ShiftCycles +
      AVM::getFixedCycles(IsAdd ? AVMCostKind::AddUpper
                                : AVMCostKind::SubUpper);
  return DecomposedCycles + 10 <= MultiplyCycles;
}

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

static SDValue getCompareWord(SDValue Value, const SDLoc &DL,
                              SelectionDAG &DAG) {
  // Comparison lowering runs after the generic trunc-of-binop combine.
  // Push the demanded word through a mask here so instruction selection can
  // use ZEXT8 instead of forming both words of a masked i32 value.
  if (ISD::isBitwiseLogicOp(Value.getOpcode())) {
    SDValue LHS = DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, Value.getOperand(0));
    SDValue RHS = DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, Value.getOperand(1));
    return DAG.getNode(Value.getOpcode(), DL, MVT::i16, LHS, RHS);
  }
  return DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, Value);
}

// Keep classification word narrowing tied to binary32 representations. In
// particular, changing the comparison shape of i64 helper words can increase
// branch/register costs even when an isolated word comparison is cheaper.
static bool isFloatRepresentation(SDValue V, unsigned Depth = 0) {
  if (V.getOpcode() == ISD::BITCAST &&
      V.getOperand(0).getValueType() == MVT::f32)
    return true;
  if (Depth == 6)
    return false;
  if (ISD::isBitwiseLogicOp(V.getOpcode()) || V.getOpcode() == ISD::SHL ||
      V.getOpcode() == ISD::SRL || V.getOpcode() == ISD::SRA ||
      V.getOpcode() == ISD::ZERO_EXTEND || V.getOpcode() == ISD::ANY_EXTEND ||
      V.getOpcode() == ISD::TRUNCATE || V.getOpcode() == ISD::EXTRACT_ELEMENT)
    for (SDValue Input : V->ops())
      if (isFloatRepresentation(Input, Depth + 1))
        return true;
  return false;
}

static std::pair<SDValue, SDValue> getAVMCompare(SDValue LHS, SDValue RHS,
                                                 ISD::CondCode CC,
                                                 const SDLoc &DL,
                                                 SelectionDAG &DAG) {
  AVMCC::CondCode TargetCond = canonicalizeCondCode(CC, LHS, RHS);

  // Classification predicates are already native compares. A branch/select
  // needs their flags, not a materialized CSET followed by another TST. Peel
  // the promoted i1 mask as well, but only when the old glue user will die.
  SDValue Bool = LHS;
  if (Bool.getOpcode() == ISD::AND &&
      isa<ConstantSDNode>(Bool.getOperand(1)) &&
      Bool.getConstantOperandVal(1) == 1)
    Bool = Bool.getOperand(0);
  if (Bool.getOpcode() == AVMISD::CSET &&
      (LHS.use_empty() || LHS.hasOneUse()) &&
      (Bool.use_empty() || Bool.hasOneUse()) && isa<ConstantSDNode>(RHS) &&
      cast<ConstantSDNode>(RHS)->isZero() &&
      (TargetCond == AVMCC::EQ || TargetCond == AVMCC::NE)) {
    unsigned Predicate = Bool.getConstantOperandVal(0);
    if (TargetCond == AVMCC::EQ)
      Predicate ^= 1;
    return {DAG.getTargetConstant(Predicate, DL, MVT::i16), Bool.getOperand(1)};
  }

  if (LHS.getValueType() == MVT::i32) {
    // Sign tests introduced by high-word extraction (for example sqrtf's
    // negative-input guard) depend only on the high word, even when the low
    // word is unknown. Do not materialize a pair or issue CMP32 for them.
    bool FloatRepresentation = isFloatRepresentation(LHS) ||
                               isFloatRepresentation(RHS);
    SDValue SignValue;
    bool Negative = false;
    const auto *LC = dyn_cast<ConstantSDNode>(LHS);
    const auto *RC = dyn_cast<ConstantSDNode>(RHS);
    if ((TargetCond == AVMCC::SLT || TargetCond == AVMCC::SGE) && RC &&
        RC->isZero()) {
      SignValue = LHS;
      Negative = TargetCond == AVMCC::SLT;
    } else if ((TargetCond == AVMCC::SLT || TargetCond == AVMCC::SGE) && LC &&
               LC->getSExtValue() == -1) {
      // Branch polarity/canonicalization also produces -1 < X or -1 >= X.
      SignValue = RHS;
      Negative = TargetCond == AVMCC::SGE;
    } else if ((TargetCond == AVMCC::ULT || TargetCond == AVMCC::UGE) && RC &&
               RC->getZExtValue() == 0x80000000) {
      SignValue = LHS;
      Negative = TargetCond == AVMCC::UGE;
    } else if ((TargetCond == AVMCC::ULT || TargetCond == AVMCC::UGE) && LC &&
               LC->getZExtValue() == 0x7fffffff) {
      SignValue = RHS;
      Negative = TargetCond == AVMCC::ULT;
    }
    if (FloatRepresentation && SignValue)
      return getAVMCompare(getI32Word(SignValue, 1, DL, DAG),
                           DAG.getConstant(0, DL, MVT::i16),
                           Negative ? ISD::SETLT : ISD::SETGE, DL, DAG);
    // InstCombine widens high-word inequalities to aligned i32 boundaries.
    // X < K:0000 and K:ffff < X depend only on the high word, as do their
    // complementary >= predicates. This also covers signed word boundaries.
    bool Ordered = TargetCond != AVMCC::EQ && TargetCond != AVMCC::NE;
    if (FloatRepresentation && Ordered &&
        ((RC && !(RC->getZExtValue() & 0xffff)) ||
                    (LC && (LC->getZExtValue() & 0xffff) == 0xffff))) {
      ISD::CondCode WordCC = TargetCond == AVMCC::SLT ? ISD::SETLT
                            : TargetCond == AVMCC::SGE ? ISD::SETGE
                            : TargetCond == AVMCC::ULT ? ISD::SETULT
                                                      : ISD::SETUGE;
      return getAVMCompare(getI32Word(LHS, 1, DL, DAG),
                           getI32Word(RHS, 1, DL, DAG), WordCC, DL, DAG);
    }
    // Aggregate fields can retain their promoted i32 type after masking.
    // Discard a common constant high word, or redundant sign extension,
    // before choosing the native word compare and its immediate form.
    KnownBits Left = DAG.computeKnownBits(LHS);
    KnownBits Right = DAG.computeKnownBits(RHS);
    APInt HighMask = APInt::getHighBitsSet(32, 16);
    APInt CommonBits = (Left.Zero & Right.Zero) | (Left.One & Right.One);
    APInt LowMask = APInt::getLowBitsSet(32, 16);
    if (FloatRepresentation && (CommonBits & LowMask) == LowMask) {
      LHS = getI32Word(LHS, 1, DL, DAG);
      RHS = getI32Word(RHS, 1, DL, DAG);
      SDValue Glue = DAG.getNode(AVMISD::CMP, DL, MVT::Glue, LHS, RHS);
      return {DAG.getTargetConstant(TargetCond, DL, MVT::i16), Glue};
    }
    bool SameHighWord = (CommonBits & HighMask) == HighMask;
    bool SignExtended = DAG.ComputeNumSignBits(LHS) >= 17 &&
                        DAG.ComputeNumSignBits(RHS) >= 17;
    if (SameHighWord || SignExtended) {
      // With equal high words, ordering depends on the unsigned low words.
      // Retain signed ordering only when both operands fit in signed i16.
      if (!SignExtended) {
        if (TargetCond == AVMCC::SLT)
          TargetCond = AVMCC::ULT;
        else if (TargetCond == AVMCC::SGE)
          TargetCond = AVMCC::UGE;
      }
      LHS = getCompareWord(LHS, DL, DAG);
      RHS = getCompareWord(RHS, DL, DAG);
    }
  }

  if (const auto *C = dyn_cast<ConstantSDNode>(LHS);
      C && LHS.getValueType() == MVT::i16 && !isa<ConstantSDNode>(RHS)) {
    bool Swap = false;
    int64_t Immediate = 0;
    switch (TargetCond) {
    case AVMCC::EQ:
    case AVMCC::NE:
      Immediate = C->getSExtValue();
      Swap = isInt<8>(Immediate);
      break;
    case AVMCC::ULT:
    case AVMCC::UGE: {
      uint64_t Value = C->getZExtValue();
      if (Value != UINT16_MAX && isInt<8>(Value + 1)) {
        Immediate = Value + 1;
        TargetCond = TargetCond == AVMCC::ULT ? AVMCC::UGE : AVMCC::ULT;
        Swap = true;
      }
      break;
    }
    case AVMCC::SLT:
    case AVMCC::SGE: {
      int64_t Value = C->getSExtValue();
      if (Value != INT16_MAX && isInt<8>(Value + 1)) {
        Immediate = Value + 1;
        TargetCond = TargetCond == AVMCC::SLT ? AVMCC::SGE : AVMCC::SLT;
        Swap = true;
      }
      break;
    }
    }
    if (Swap) {
      LHS = RHS;
      RHS = DAG.getSignedConstant(Immediate, DL, MVT::i16);
    }
  }

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
  if (SDValue P = lowerClassCompare(LHS, RHS, CC, DL, DAG)) {
    std::tie(TargetCC, Glue) = getAVMCompare(
        P, DAG.getConstant(0, DL, MVT::i16), ISD::SETNE, DL, DAG);
    return true;
  }
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
  if (SDValue P = lowerClassCompare(LHS, RHS, CC, DL, DAG))
    return P;
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
  setOperationAction(ISD::UADDSAT, MVT::i16, Custom);
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
       {ISD::ADD, ISD::SUB, ISD::XOR, ISD::BSWAP})
    setOperationAction(Opcode, MVT::i32, Legal);
  setOperationAction(ISD::AND, MVT::i32, Custom);
  setOperationAction(ISD::OR, MVT::i32, Custom);
  setOperationAction(ISD::BUILD_PAIR, MVT::i32, Legal);
  // EXTRACT_ELEMENT defaults to expansion even when marked Legal. Keep native
  // word projections for instruction selection, simplifying them as operands
  // are legalized into pairs.
  setOperationAction(ISD::EXTRACT_ELEMENT, MVT::i16, Custom);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i32, Legal);
  setOperationAction(ISD::MUL, MVT::i32, Custom);
  for (unsigned Opcode : {ISD::UDIV, ISD::UREM, ISD::SDIV, ISD::SREM})
    setOperationAction(Opcode, MVT::i32, LibCall);
  for (unsigned Opcode : {ISD::SHL, ISD::SRL, ISD::SRA})
    setOperationAction(Opcode, MVT::i32, Custom);
  setTargetDAGCombine(
      {ISD::LOAD, ISD::MUL, ISD::SUB, ISD::BUILD_PAIR, ISD::EXTRACT_ELEMENT,
       ISD::TRUNCATE, ISD::UMIN});
  for (unsigned Opcode : {ISD::ROTL, ISD::ROTR}) {
    setOperationAction(Opcode, MVT::i16, Expand);
    setOperationAction(Opcode, MVT::i32, Custom);
  }
  for (unsigned Opcode :
       {ISD::MULHU, ISD::MULHS, ISD::UMUL_LOHI, ISD::SMUL_LOHI})
    setOperationAction(Opcode, MVT::i16, Custom);
  for (unsigned Opcode :
       {ISD::MULHU, ISD::MULHS, ISD::UMUL_LOHI, ISD::SMUL_LOHI, ISD::SHL_PARTS,
        ISD::SRL_PARTS, ISD::SRA_PARTS})
    setOperationAction(Opcode, MVT::i32, Expand);
  for (unsigned Opcode : {ISD::MUL, ISD::UDIV, ISD::UREM, ISD::SDIV, ISD::SREM,
                          ISD::SHL, ISD::SRL, ISD::SRA})
    setOperationAction(Opcode, MVT::i64, LibCall);
  // i32 pairs use the generic pointer-output divmod libcall expansion. i64
  // needs custom type legalization because it is not a legal register type.
  for (unsigned Opcode : {ISD::UDIVREM, ISD::SDIVREM}) {
    setOperationAction(Opcode, MVT::i32, Expand);
    setOperationAction(Opcode, MVT::i64, Custom);
  }

  setLibcallImpl(RTLIB::MUL_I32, RTLIB::impl___avm_mulsi3);
  setLibcallImpl(RTLIB::UDIV_I32, RTLIB::impl___avm_udivsi3);
  setLibcallImpl(RTLIB::UREM_I32, RTLIB::impl___avm_umodsi3);
  setLibcallImpl(RTLIB::SDIV_I32, RTLIB::impl___avm_divsi3);
  setLibcallImpl(RTLIB::SREM_I32, RTLIB::impl___avm_modsi3);
  setLibcallImpl(RTLIB::UDIVREM_I32, RTLIB::impl___avm_udivmodsi4);
  setLibcallImpl(RTLIB::SDIVREM_I32, RTLIB::impl___avm_divmodsi4);
  setLibcallImpl(RTLIB::SHL_I32, RTLIB::impl___avm_ashlsi3);
  setLibcallImpl(RTLIB::SRL_I32, RTLIB::impl___avm_lshrsi3);
  setLibcallImpl(RTLIB::SRA_I32, RTLIB::impl___avm_ashrsi3);
  setLibcallImpl(RTLIB::MUL_I64, RTLIB::impl___avm_muldi3);
  setLibcallImpl(RTLIB::UDIV_I64, RTLIB::impl___avm_udivdi3);
  setLibcallImpl(RTLIB::UREM_I64, RTLIB::impl___avm_umoddi3);
  setLibcallImpl(RTLIB::SDIV_I64, RTLIB::impl___avm_divdi3);
  setLibcallImpl(RTLIB::SREM_I64, RTLIB::impl___avm_moddi3);
  setLibcallImpl(RTLIB::UDIVREM_I64, RTLIB::impl___avm_udivmoddi4);
  setLibcallImpl(RTLIB::SDIVREM_I64, RTLIB::impl___avm_divmoddi4);
  setLibcallImpl(RTLIB::SHL_I64, RTLIB::impl___avm_ashldi3);
  setLibcallImpl(RTLIB::SRL_I64, RTLIB::impl___avm_lshrdi3);
  setLibcallImpl(RTLIB::SRA_I64, RTLIB::impl___avm_ashrdi3);
  setLibcallImpl(RTLIB::ADD_F32, RTLIB::impl___addsf3);
  setLibcallImpl(RTLIB::SUB_F32, RTLIB::impl___subsf3);
  setLibcallImpl(RTLIB::MUL_F32, RTLIB::impl___mulsf3);
  setLibcallImpl(RTLIB::DIV_F32, RTLIB::impl___divsf3);
  setLibcallImpl(RTLIB::SQRT_F32, RTLIB::impl_sqrtf);
  setLibcallImpl(RTLIB::SIN_F32, RTLIB::impl_sinf);
  setLibcallImpl(RTLIB::COS_F32, RTLIB::impl_cosf);
  setLibcallImpl(RTLIB::TAN_F32, RTLIB::impl_tanf);
  setLibcallImpl(RTLIB::ATAN2_F32, RTLIB::impl_atan2f);
  setLibcallImpl(RTLIB::EXP_F32, RTLIB::impl_expf);
  setLibcallImpl(RTLIB::LOG_F32, RTLIB::impl_logf);
  setLibcallImpl(RTLIB::LOG2_F32, RTLIB::impl_log2f);
  setLibcallImpl(RTLIB::LOG10_F32, RTLIB::impl_log10f);
  setLibcallImpl(RTLIB::POW_F32, RTLIB::impl_powf);
  setLibcallImpl(RTLIB::REM_F32, RTLIB::impl_fmodf);
  setLibcallImpl(RTLIB::TRUNC_F32, RTLIB::impl_truncf);
  setLibcallImpl(RTLIB::FLOOR_F32, RTLIB::impl_floorf);
  setLibcallImpl(RTLIB::CEIL_F32, RTLIB::impl_ceilf);
  setLibcallImpl(RTLIB::ROUND_F32, RTLIB::impl_roundf);
  setLibcallImpl(RTLIB::FMIN_F32, RTLIB::impl_fminf);
  setLibcallImpl(RTLIB::FMAX_F32, RTLIB::impl_fmaxf);
  setLibcallImpl(RTLIB::FPTOSINT_F32_I32, RTLIB::impl___fixsfsi);
  setLibcallImpl(RTLIB::FPTOUINT_F32_I32, RTLIB::impl___fixunssfsi);
  setLibcallImpl(RTLIB::SINTTOFP_I32_F32, RTLIB::impl___floatsisf);
  setLibcallImpl(RTLIB::UINTTOFP_I32_F32, RTLIB::impl___floatunsisf);
  setLibcallImpl(RTLIB::SINTTOFP_I64_F32, RTLIB::impl___floatdisf);
  setLibcallImpl(RTLIB::UINTTOFP_I64_F32, RTLIB::impl___floatundisf);
  setLibcallImpl(RTLIB::FPTOSINT_F32_I64, RTLIB::impl___fixsfdi);
  setLibcallImpl(RTLIB::FPTOUINT_F32_I64, RTLIB::impl___fixunssfdi);
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
       {ISD::FADD, ISD::FSUB, ISD::FMUL, ISD::FDIV, ISD::FNEG, ISD::FABS, ISD::FSQRT, ISD::FTRUNC, ISD::FFLOOR,
        ISD::FCEIL, ISD::FROUND, ISD::FREM, ISD::FMA})
    setOperationAction(Opcode, MVT::f32, Legal);
  for (unsigned Opcode :
       {ISD::FSIN, ISD::FCOS, ISD::FATAN2, ISD::FTAN, ISD::FEXP, ISD::FLOG,
        ISD::FLOG2, ISD::FLOG10, ISD::FPOW, ISD::FMINNUM,
        ISD::FMAXNUM})
    setOperationAction(Opcode, MVT::f32, LibCall);
  setOperationAction(ISD::INTRINSIC_WO_CHAIN, MVT::Other, Custom);
  setOperationAction(ISD::FCOPYSIGN, MVT::f32, Expand);
  for (unsigned Opcode : {ISD::STRICT_FADD, ISD::STRICT_FSUB, ISD::STRICT_FMUL,
                          ISD::STRICT_FDIV, ISD::STRICT_FSQRT})
    setOperationAction(Opcode, MVT::f32, Custom);
  for (MVT VT : {MVT::i16, MVT::i32}) {
    setOperationAction(ISD::SINT_TO_FP, VT, Legal);
    setOperationAction(ISD::UINT_TO_FP, VT, Legal);
    setOperationAction(ISD::STRICT_SINT_TO_FP, VT, Custom);
    setOperationAction(ISD::STRICT_UINT_TO_FP, VT, Custom);
    setOperationAction(ISD::FP_TO_SINT, VT, Legal);
    setOperationAction(ISD::FP_TO_UINT, VT, Legal);
    setOperationAction(ISD::STRICT_FP_TO_SINT, VT, Custom);
    setOperationAction(ISD::STRICT_FP_TO_UINT, VT, Custom);
  }
  setOperationAction(ISD::SINT_TO_FP, MVT::i64, LibCall);
  setOperationAction(ISD::UINT_TO_FP, MVT::i64, LibCall);
  setOperationAction(ISD::STRICT_FSETCC, MVT::f32, Custom);
  setOperationAction(ISD::STRICT_FSETCCS, MVT::f32, Custom);
  setOperationAction(ISD::IS_FPCLASS, MVT::f32, Custom);
  setOperationAction(ISD::ADDRSPACECAST, MVT::i16, Custom);
  setOperationAction(ISD::ADDRSPACECAST, MVT::i32, Custom);
  // The generic expansion reads SP, subtracts the runtime size, and writes SP
  // back. AVM's GETSP/SETSP copies provide exactly these operations.
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i16, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  for (unsigned Opcode : {ISD::CTPOP, ISD::CTLZ, ISD::CTLZ_ZERO_UNDEF,
                          ISD::CTTZ, ISD::CTTZ_ZERO_UNDEF})
    setOperationAction(Opcode, MVT::i16, Expand);
  // i32 is also a legal register type without native bit-count instructions.
  // The generic CTLZ expansion introduces CTPOP, so expand that as well.
  for (unsigned Opcode : {ISD::CTPOP})
    setOperationAction(Opcode, MVT::i32, Expand);
  setOperationAction(ISD::CTLZ, MVT::i32, Custom);
  setOperationAction(ISD::CTLZ_ZERO_UNDEF, MVT::i32, Custom);
  setOperationAction(ISD::CTTZ, MVT::i32, Custom);
  setOperationAction(ISD::CTTZ_ZERO_UNDEF, MVT::i32, Custom);
  setOperationAction(ISD::CTTZ, MVT::i64, Custom);
  setOperationAction(ISD::CTTZ_ZERO_UNDEF, MVT::i64, Custom);
  setOperationAction(ISD::CTLZ, MVT::i64, Custom);
  setOperationAction(ISD::CTLZ_ZERO_UNDEF, MVT::i64, Custom);
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::VAARG, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Custom);
  setOperationAction(ISD::INTRINSIC_VOID, MVT::i8, Custom);
  setMaxAtomicSizeInBitsSupported(32);

  setLoadExtAction({ISD::EXTLOAD, ISD::ZEXTLOAD, ISD::SEXTLOAD}, MVT::i16,
                   MVT::i1, Promote);
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
  setIndexedLoadAction(ISD::PRE_DEC, MVT::i8, Legal);
  setIndexedLoadAction(ISD::PRE_DEC, MVT::i16, Legal);
  setIndexedStoreAction(ISD::PRE_DEC, MVT::i8, Legal);
  setIndexedStoreAction(ISD::PRE_DEC, MVT::i16, Legal);

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
  if (Opcode == AVMISD::TAIL_CALL)
    return "AVMISD::TAIL_CALL";
  if (Opcode == AVMISD::CMOV)
    return "AVMISD::CMOV";
  if (Opcode == AVMISD::CMP)
    return "AVMISD::CMP";
  if (Opcode == AVMISD::CMPI)
    return "AVMISD::CMPI";
  if (Opcode == AVMISD::CSET)
    return "AVMISD::CSET";
  if (Opcode == AVMISD::BOOL)
    return "AVMISD::BOOL";
  if (Opcode == AVMISD::FCMP)
    return "AVMISD::FCMP";
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
  if (Opcode == AVMISD::BUILD_HI16)
    return "AVMISD::BUILD_HI16";
  if (Opcode == AVMISD::LSR32_1)
    return "AVMISD::LSR32_1";
  if (Opcode == AVMISD::SRA32_1)
    return "AVMISD::SRA32_1";
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

bool AVMTargetLowering::getPreIndexedAddressParts(SDNode *N, SDValue &Base,
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

  if (AddressSpace != 0 || (MemoryVT != MVT::i8 && MemoryVT != MVT::i16) ||
      Pointer.getOpcode() != ISD::ADD)
    return false;

  int64_t Width = MemoryVT.getStoreSize();
  const auto *Decrement = dyn_cast<ConstantSDNode>(Pointer.getOperand(1));
  if (!Decrement || Decrement->getSExtValue() != -Width)
    return false;

  Base = Pointer.getOperand(0);
  Offset = DAG.getSignedConstant(-Width, SDLoc(N), Base.getValueType());
  AM = ISD::PRE_DEC;
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

static SDValue getAVMSelect(SDValue True, SDValue False, SDValue CC,
                            SDValue Glue, const SDLoc &DL, SelectionDAG &DAG) {
  EVT VT = True.getValueType();
  if (VT == MVT::i32 || VT == MVT::f32) {
    auto GetBits = [](SDValue Value, uint32_t &Bits) {
      if (const auto *C = dyn_cast<ConstantSDNode>(Value)) {
        Bits = C->getZExtValue();
        return true;
      }
      if (const auto *C = dyn_cast<ConstantFPSDNode>(Value)) {
        Bits = C->getValueAPF().bitcastToAPInt().getZExtValue();
        return true;
      }
      return false;
    };
    uint32_t TrueBits, FalseBits;
    if (GetBits(True, TrueBits) && GetBits(False, FalseBits) &&
        ((TrueBits & 0xffff) == (FalseBits & 0xffff) ||
         (TrueBits >> 16) == (FalseBits >> 16))) {
      // A wide CMOV keeps two complete register pairs live and selects both
      // words. Share an identical word and select only the differing word.
      // Use the exact representation for f32, including signed zeros and NaNs.
      SDValue Words[2];
      for (unsigned I = 0; I != 2; ++I) {
        SDValue T = DAG.getConstant((TrueBits >> (16 * I)) & 0xffff, DL,
                                    MVT::i16);
        SDValue F = DAG.getConstant((FalseBits >> (16 * I)) & 0xffff, DL,
                                    MVT::i16);
        Words[I] = T == F ? T : DAG.getNode(AVMISD::CMOV, DL, MVT::i16, T, F,
                                           CC, Glue);
      }
      SDValue Pair = DAG.getNode(ISD::BUILD_PAIR, DL, MVT::i32, Words);
      return VT == MVT::f32 ? DAG.getNode(ISD::BITCAST, DL, VT, Pair) : Pair;
    }
  }
  return DAG.getNode(AVMISD::CMOV, DL, VT, True, False, CC, Glue);
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
      return getAVMSelect(Op.getOperand(2), Op.getOperand(3), TargetCC, Glue,
                           DL, DAG);
    CompareLHS = getAVMFloatSetCC(CompareLHS, CompareRHS, CC, DL, DAG);
    CompareRHS = DAG.getConstant(0, DL, MVT::i16);
    CC = ISD::SETNE;
  }
  auto [TargetCC, Glue] = getAVMCompare(CompareLHS, CompareRHS, CC, DL, DAG);
  return getAVMSelect(Op.getOperand(2), Op.getOperand(3), TargetCC, Glue, DL,
                       DAG);
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
  return getAVMSelect(Op.getOperand(1), Op.getOperand(2), TargetCC, Glue, DL,
                       DAG);
}

SDValue AVMTargetLowering::LowerISFPClass(SDValue Op, SelectionDAG &DAG) const {
  return lowerFPClass(Op.getOperand(0), Op.getConstantOperandVal(1), SDLoc(Op),
                      DAG);
}

static SDValue lowerFPClass(SDValue Value, unsigned Mask, const SDLoc &DL,
                            SelectionDAG &DAG) {
  Mask &= 0x3ff;
  auto C = [&](unsigned V) { return DAG.getConstant(V, DL, MVT::i16); };
  if (!Mask || Mask == 0x3ff)
    return C(Mask != 0);

  SDValue Bits = DAG.getNode(ISD::BITCAST, DL, MVT::i32, Value);
  SDValue Hi = getI32Word(Bits, 1, DL, DAG);
  auto Bin = [&](unsigned Opcode, SDValue A, SDValue B) {
    return DAG.getNode(Opcode, DL, MVT::i16, A, B);
  };
  auto And = [&](SDValue A, SDValue B) { return Bin(ISD::AND, A, B); };
  auto Or = [&](SDValue A, SDValue B) { return Bin(ISD::OR, A, B); };
  auto Test = [&](SDValue A, unsigned B, ISD::CondCode CC) {
    // Target compares keep DAGCombine from recognizing these integer tests as
    // floating classification again. All arithmetic below is native i16.
    return getAVMCSet(A, C(B), CC, DL, DAG);
  };
  auto Not = [&](SDValue P) {
    if (P.getOpcode() == AVMISD::CSET && P.use_empty())
      return DAG.getNode(AVMISD::CSET, DL, MVT::i16,
                         DAG.getTargetConstant(P.getConstantOperandVal(0) ^ 1,
                                                DL, MVT::i16),
                         P.getOperand(1));
    return Bin(ISD::XOR, P, C(1));
  };
  SDValue Exp = And(Hi, C(0x7f80));
  if (Mask == 0x1f8 || Mask == 0x207)
    return Test(Exp, 0x7f80, Mask == 0x1f8 ? ISD::SETNE : ISD::SETEQ);
  auto Normal = [&] {
    // One unsigned interval excludes both exponent endpoints.
    return Test(Bin(ISD::SUB, Exp, C(0x80)), 0x7f00, ISD::SETULT);
  };
  if (Mask == 0x108)
    return Normal();
  if (Mask == 0x2f7)
    return Not(Normal());
  if (Mask == 0xf0)
    return Test(Exp, 0, ISD::SETEQ);
  if (Mask == 0x30 || Mask == 0xc0)
    return Test(And(Hi, C(0xff80)), Mask == 0x30 ? 0x8000 : 0,
                ISD::SETEQ);
  if (llvm::popcount(Mask) > 5)
    return Not(lowerFPClass(Value, Mask ^ 0x3ff, DL, DAG));

  // Share sign-independent category tests, adding a sign test only when the
  // mask selects one sign. Low-word extraction is lazy: finite, normal, and
  // quiet-NaN tests never reference it. A taken native branch costs 132
  // cycles. An eager low-word OR costs 17; even NaN's low-word test plus
  // merge costs only 55 with native BOOL, so branching around it is slower.
  SDValue ShiftedHigh;
  auto AbsHigh = [&] {
    if (!ShiftedHigh)
      // Drop the sign with native doubling, cheaper than materializing
      // 0x7fff and ANDing it. The low bit is now free.
      ShiftedHigh = Bin(ISD::SHL, Hi, C(1));
    return ShiftedHigh;
  };
  auto Lo = [&] { return getI32Word(Bits, 0, DL, DAG); };
  auto Nonzero = [&](SDValue V) {
    KnownBits Known = DAG.computeKnownBits(V);
    if (Known.Zero.isAllOnes())
      return C(0);
    if (!Known.One.isZero())
      return C(1);
    // Existing native BOOL costs 38 cycles, versus 80 for TST16 + CSET.
    return DAG.getNode(AVMISD::BOOL, DL, MVT::i16, V);
  };
  SDValue Result = C(0);
  auto Add = [&](SDValue P) { Result = Or(Result, P); };
  if (Mask & 3) {
    SDValue P;
    if ((Mask & 3) == 2) {
      // The quiet bit itself proves a nonzero fraction.
      P = Test(And(Hi, C(0x7fc0)), 0x7fc0, ISD::SETEQ);
    } else {
      // Fold all low fraction bits into the vacant low bit of the shifted
      // high word, then compare unsigned against infinity. Normal values need
      // no separate exponent predicate and no fraction-mask temporary.
      P = Test(Or(AbsHigh(), Nonzero(Lo())), 0xff00, ISD::SETUGT);
      if ((Mask & 3) == 1)
        P = And(P, Test(And(Hi, C(0x40)), 0, ISD::SETEQ));
    }
    Add(P);
  }
  if (Mask & 0x204) {
    unsigned Signs = Mask & 0x204;
    SDValue H = Signs == 0x204 ? AbsHigh() : Hi;
    unsigned Limit = Signs == 0x204 ? 0xff00 : Signs == 4 ? 0xff80 : 0x7f80;
    // One zero test, including the sign directly for a single infinity.
    Add(Test(Or(Bin(ISD::XOR, H, C(Limit)),
                Lo()), 0, ISD::SETEQ));
  }
  if (Mask & 0x108) {
    unsigned Signs = Mask & 0x108;
    if (Signs == 0x108)
      Add(Normal());
    else
      // Normal high words occupy a contiguous interval for each sign. The
      // interval includes every fraction, so no fraction or sign test is needed.
      Add(Test(Bin(ISD::SUB, Hi, C(Signs == 8 ? 0x8080 : 0x80)),
               0x7f00, ISD::SETULT));
  }
  if (Mask & 0x090) {
    unsigned Signs = Mask & 0x090;
    SDValue H = Signs == 0x90 ? AbsHigh() : Hi;
    if (Signs == 0x10)
      H = Bin(ISD::XOR, H, C(0x8000));
    Add(And(Test(H, Signs == 0x90 ? 0x100 : 0x80, ISD::SETULT),
            Nonzero(Or(AbsHigh(), Lo()))));
  }
  if (Mask & 0x060) {
    unsigned Signs = Mask & 0x60;
    SDValue H = Signs == 0x60 ? AbsHigh() : Hi;
    if (Signs == 0x20)
      H = Bin(ISD::XOR, H, C(0x8000));
    Add(Test(Or(H, Lo()), 0, ISD::SETEQ));
  }
  return Result;
}

static SDValue lowerClassCompare(SDValue LHS, SDValue RHS, ISD::CondCode CC,
                                 const SDLoc &DL, SelectionDAG &DAG) {
  if (isa<ConstantFPSDNode>(LHS)) {
    std::swap(LHS, RHS);
    CC = ISD::getSetCCSwappedOperands(CC);
  }
  const auto *C = dyn_cast<ConstantFPSDNode>(RHS);
  bool Self = LHS == RHS;
  if (!Self && (!C || C->getValueAPF().isNaN()))
    return SDValue();
  if (CC == ISD::SETUO || CC == ISD::SETO)
    return lowerFPClass(LHS, CC == ISD::SETUO ? 3 : 1020, DL, DAG);
  unsigned EqualMask;
  if (Self)
    EqualMask = 1020;
  else if (C->isZero())
    EqualMask = 96;
  else if (C->getValueAPF().isInfinity())
    EqualMask = C->isNegative() ? 4 : 512;
  else
    return SDValue();
  unsigned Mask;
  switch (CC) {
  case ISD::SETEQ: case ISD::SETOEQ: Mask = EqualMask; break;
  case ISD::SETUEQ: Mask = EqualMask | 3; break;
  case ISD::SETNE: case ISD::SETUNE: Mask = 1023 ^ EqualMask; break;
  case ISD::SETONE: Mask = 1020 ^ EqualMask; break;
  default: return SDValue();
  }
  return lowerFPClass(LHS, Mask, DL, DAG);
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

// Keep word construction visible to legalization. Rebuilding a pair with wide
// extensions and ORs hides zero halves and forces register-pair temporaries for
// byte packing, masks, and constant shifts.
static SDValue getI32Word(SDValue Value, unsigned Index, const SDLoc &DL,
                         SelectionDAG &DAG) {
  assert(Value.getValueType() == MVT::i32 && Index < 2 && "expected i32 word");
  if (Value.getOpcode() == ISD::AssertZext ||
      Value.getOpcode() == ISD::AssertSext) {
    unsigned Bits = cast<VTSDNode>(Value.getOperand(1))->getVT().getSizeInBits();
    if (Bits <= 16 * Index) {
      if (Value.getOpcode() == ISD::AssertZext)
        return DAG.getConstant(0, DL, MVT::i16);
      return DAG.getNode(ISD::SRA, DL, MVT::i16,
                         getI32Word(Value.getOperand(0), 0, DL, DAG),
                         DAG.getConstant(15, DL, MVT::i16));
    }
    SDValue Word = getI32Word(Value.getOperand(0), Index, DL, DAG);
    if (Bits >= 16 * (Index + 1))
      return Word;
    EVT NarrowVT = EVT::getIntegerVT(*DAG.getContext(), Bits - 16 * Index);
    return DAG.getNode(Value.getOpcode(), DL, MVT::i16, Word,
                       DAG.getValueType(NarrowVT));
  }
  if (Value.getOpcode() == ISD::BUILD_PAIR)
    return Value.getOperand(Index);
  if (Value.getOpcode() == AVMISD::BUILD_HI16)
    return Index ? Value.getOperand(0) : DAG.getConstant(0, DL, MVT::i16);
  if (Value.getOpcode() == ISD::SIGN_EXTEND_INREG) {
    unsigned Bits = cast<VTSDNode>(Value.getOperand(1))->getVT().getSizeInBits();
    if (Bits <= 16 * Index)
      return DAG.getNode(ISD::SRA, DL, MVT::i16,
                         getI32Word(Value, 0, DL, DAG),
                         DAG.getConstant(15, DL, MVT::i16));
    SDValue Word = getI32Word(Value.getOperand(0), Index, DL, DAG);
    if (Bits >= 16 * (Index + 1))
      return Word;
    EVT NarrowVT = EVT::getIntegerVT(*DAG.getContext(), Bits - 16 * Index);
    return DAG.getNode(ISD::SIGN_EXTEND_INREG, DL, MVT::i16, Word,
                       DAG.getValueType(NarrowVT));
  }
  if (ISD::isBitwiseLogicOp(Value.getOpcode()) && Value.hasOneUse())
    return DAG.getNode(Value.getOpcode(), DL, MVT::i16,
                       getI32Word(Value.getOperand(0), Index, DL, DAG),
                       getI32Word(Value.getOperand(1), Index, DL, DAG));
  if (const auto *C = dyn_cast<ConstantSDNode>(Value))
    return DAG.getConstant((C->getZExtValue() >> (16 * Index)) & 0xffff, DL,
                           MVT::i16);

  if ((Value.getOpcode() == ISD::ZERO_EXTEND ||
       Value.getOpcode() == ISD::SIGN_EXTEND ||
       Value.getOpcode() == ISD::ANY_EXTEND) &&
      Value.getOperand(0).getValueSizeInBits() <= 16) {
    SDValue Low = DAG.getNode(Value.getOpcode(), DL, MVT::i16,
                              Value.getOperand(0));
    if (!Index)
      return Low;
    return Value.getOpcode() == ISD::SIGN_EXTEND
               ? DAG.getNode(ISD::SRA, DL, MVT::i16, Low,
                             DAG.getConstant(15, DL, MVT::i16))
               : DAG.getConstant(0, DL, MVT::i16);
  }

  KnownBits Known = DAG.computeKnownBits(Value).extractBits(16, 16 * Index);
  if (Known.isConstant())
    return DAG.getConstant(Known.getConstant(), DL, MVT::i16);
  return DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, Value,
                     DAG.getConstant(Index, DL, MVT::i16));
}

static std::pair<SDValue, SDValue> getI32Words(SDValue Value, const SDLoc &DL,
                                           SelectionDAG &DAG) {
  return {getI32Word(Value, 0, DL, DAG), getI32Word(Value, 1, DL, DAG)};
}

SDValue AVMTargetLowering::BuildI32FromWords(SDValue Low, SDValue High,
                                             const SDLoc &DL,
                                             SelectionDAG &DAG) const {
  assert(Low.getValueType() == MVT::i16 && High.getValueType() == MVT::i16 &&
         "expected i16 words");

  if (isNullConstant(Low))
    return DAG.getNode(AVMISD::BUILD_HI16, DL, MVT::i32, High);
  if (isNullConstant(High))
    return DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i32, Low);
  if (High.getOpcode() == ISD::SRA && High.getOperand(0) == Low)
    if (const auto *Count = dyn_cast<ConstantSDNode>(High.getOperand(1));
        Count && Count->getZExtValue() == 15)
      return DAG.getNode(ISD::SIGN_EXTEND, DL, MVT::i32, Low);
  return DAG.getNode(ISD::BUILD_PAIR, DL, MVT::i32, Low, High);
}

SDValue AVMTargetLowering::LowerI32Bitwise(SDValue Op,
                                        SelectionDAG &DAG) const {
  SDLoc DL(Op);
  // A full-width bitwise operation is already two native operations tied to
  // one pair. Splitting is useful when packing exposes an identity operation
  // in at least one word; otherwise it needlessly separates the live ranges.
  if (Op.getOpcode() == ISD::AND) {
    KnownBits Known = DAG.computeKnownBits(Op);
    if (!Known.Zero.getLoBits(16).isAllOnes() &&
        !Known.Zero.getHiBits(16).isAllOnes())
      return Op;
  }
  auto [LeftLow, LeftHigh] = getI32Words(Op.getOperand(0), DL, DAG);
  auto [RightLow, RightHigh] = getI32Words(Op.getOperand(1), DL, DAG);
  if (Op.getOpcode() == ISD::OR && !isNullConstant(LeftLow) &&
      !isNullConstant(LeftHigh) && !isNullConstant(RightLow) &&
      !isNullConstant(RightHigh))
    return Op;
  SDValue Low = DAG.getNode(Op.getOpcode(), DL, MVT::i16, LeftLow, RightLow);
  SDValue High = DAG.getNode(Op.getOpcode(), DL, MVT::i16, LeftHigh, RightHigh);
  return BuildI32FromWords(Low, High, DL, DAG);
}

SDValue AVMTargetLowering::LowerI32Rotate(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Value = Op.getOperand(0);
  SDValue Count = Op.getOperand(1);
  bool IsLeft = Op.getOpcode() == ISD::ROTL;
  if (const auto *C = dyn_cast<ConstantSDNode>(Count)) {
    // Keep constant rotations in native code through the constant-shift
    // lowering, including a pure word exchange for a rotation by 16.
    unsigned Amount = C->getZExtValue() & 31;
    if (!Amount)
      return Value;
    if (!IsLeft)
      Amount = 32 - Amount;
    SDValue Left = DAG.getNode(ISD::SHL, DL, MVT::i32, Value,
                               DAG.getConstant(Amount, DL, MVT::i16));
    SDValue Right = DAG.getNode(ISD::SRL, DL, MVT::i32, Value,
                                DAG.getConstant(32 - Amount, DL, MVT::i16));
    return DAG.getNode(ISD::OR, DL, MVT::i32, Left, Right);
  }

  // Rotation only depends on the low five count bits. The target helpers
  // accept a native word count and normalize it modulo 32 themselves.
  Count = DAG.getZExtOrTrunc(Count, DL, MVT::i16);
  ArgListTy Args;
  for (SDValue Arg : {Value, Count}) {
    ArgListEntry Entry(Arg, Arg.getValueType().getTypeForEVT(*DAG.getContext()));
    Entry.IsZExt = true;
    Args.push_back(Entry);
  }
  SDValue Callee = DAG.getExternalSymbol(
      IsLeft ? "__avm_rotlsi3" : "__avm_rotrsi3",
      getPointerTy(DAG.getDataLayout()));
  CallLoweringInfo CLI(DAG);
  CLI.setDebugLoc(DL)
      .setChain(DAG.getEntryNode())
      .setLibCallee(CallingConv::C, Type::getInt32Ty(*DAG.getContext()), Callee,
                    std::move(Args))
      .setZExtResult(true);
  return LowerCallTo(CLI).first;
}

SDValue AVMTargetLowering::LowerI32Shift(SDValue Op, SelectionDAG &DAG) const {
  assert(Op.getValueType() == MVT::i32 && "expected i32 shift");

  SDLoc DL(Op);
  SDValue Value = Op.getOperand(0);
  const auto *CountNode = dyn_cast<ConstantSDNode>(Op.getOperand(1));
  auto EmitLibCall = [&]() {
    RTLIB::Libcall LC = Op.getOpcode() == ISD::SHL   ? RTLIB::SHL_I32
                        : Op.getOpcode() == ISD::SRL ? RTLIB::SRL_I32
                                                     : RTLIB::SRA_I32;
    MakeLibCallOptions CallOptions;
    SDValue Ops[] = {Value, Op.getOperand(1)};
    return makeLibCall(DAG, LC, MVT::i32, Ops, CallOptions, DL).first;
  };

  if (!CountNode &&
      DAG.computeKnownBits(Op.getOperand(1)).getMaxValue().ult(32)) {
    // Prove the full count range before narrowing it: wide, unproven counts
    // retain the helper. Known zero/sign words fold out of these native word
    // shifts, as do carries which the input's known bits cannot reach.
    KnownBits KnownCount = DAG.computeKnownBits(Op.getOperand(1));
    bool SmallOnly = KnownCount.getMaxValue().ult(16);
    bool LargeOnly = KnownCount.getMinValue().uge(16);
    SDValue Count = DAG.getZExtOrTrunc(Op.getOperand(1), DL, MVT::i16);
    SDValue WordCount = SmallOnly
                            ? Count
                            : DAG.getNode(ISD::AND, DL, MVT::i16, Count,
                                          DAG.getConstant(15, DL, MVT::i16));
    auto [Low, High] = getI32Words(Value, DL, DAG);
    SDValue Zero = DAG.getConstant(0, DL, MVT::i16);
    bool IsLeft = Op.getOpcode() == ISD::SHL;
    bool IsArithmetic = Op.getOpcode() == ISD::SRA;
    SDValue Sign = IsArithmetic ? DAG.getNode(ISD::SRA, DL, MVT::i16, High,
                                              DAG.getConstant(15, DL, MVT::i16))
                                : Zero;
    SDValue WordBit;
    if (!SmallOnly && !LargeOnly)
      WordBit = DAG.getNode(ISD::AND, DL, MVT::i16, Count,
                            DAG.getConstant(16, DL, MVT::i16));
    if (IsArithmetic && DAG.ComputeNumSignBits(Value) >= 17) {
      // A sign-extended word needs no cross-word carry. Counts >= 16 leave
      // only its sign, while smaller counts use one native arithmetic shift.
      SDValue Word = LargeOnly
                         ? Sign
                         : DAG.getNode(ISD::SRA, DL, MVT::i16, Low, WordCount);
      if (!SmallOnly && !LargeOnly)
        Word = DAG.getSelectCC(DL, WordBit, Zero, Sign, Word, ISD::SETNE);
      return DAG.getNode(ISD::SIGN_EXTEND, DL, MVT::i32, Word);
    }
    // The word which survives a count >= 16 is also one of the words needed
    // for a count < 16, so a range crossing 16 shares this shift.
    SDValue Shifted = DAG.getNode(IsLeft         ? ISD::SHL
                                  : IsArithmetic ? ISD::SRA
                                                 : ISD::SRL,
                                  DL, MVT::i16, IsLeft ? Low : High, WordCount);
    SDValue LargeLow = IsLeft ? Zero : Shifted;
    SDValue LargeHigh = IsLeft ? Shifted : Sign;
    if (LargeOnly)
      return BuildI32FromWords(LargeLow, LargeHigh, DL, DAG);

    unsigned CarryOpcode = IsLeft ? ISD::SRL : ISD::SHL;
    SDValue CarryInput = IsLeft ? Low : High;
    SDValue CarryCount;
    if (DAG.isKnownNeverZero(WordCount)) {
      CarryCount = DAG.getNode(ISD::SUB, DL, MVT::i16,
                               DAG.getConstant(16, DL, MVT::i16), WordCount);
    } else {
      // LLVM i16 shifts cannot express a count of 16, and AVM masks counts
      // modulo 16. Split off one bit to make a zero count produce zero carry:
      // (low >> 1) >> (15-n), or (high << 1) << (15-n) for right shifts.
      CarryInput = DAG.getNode(CarryOpcode, DL, MVT::i16, CarryInput,
                               DAG.getConstant(1, DL, MVT::i16));
      CarryCount = DAG.getNode(ISD::XOR, DL, MVT::i16, WordCount,
                               DAG.getConstant(15, DL, MVT::i16));
    }
    SDValue Carry =
        DAG.getNode(CarryOpcode, DL, MVT::i16, CarryInput, CarryCount);
    SDValue Other = DAG.getNode(IsLeft ? ISD::SHL : ISD::SRL, DL, MVT::i16,
                                IsLeft ? High : Low, WordCount);
    Other = DAG.getNode(ISD::OR, DL, MVT::i16, Other, Carry);
    SDValue SmallLow = IsLeft ? Shifted : Other;
    SDValue SmallHigh = IsLeft ? Other : Shifted;
    if (SmallOnly)
      return BuildI32FromWords(SmallLow, SmallHigh, DL, DAG);

    SDValue NewLow =
        DAG.getSelectCC(DL, WordBit, Zero, LargeLow, SmallLow, ISD::SETNE);
    SDValue NewHigh =
        DAG.getSelectCC(DL, WordBit, Zero, LargeHigh, SmallHigh, ISD::SETNE);
    return BuildI32FromWords(NewLow, NewHigh, DL, DAG);
  }

  if (!CountNode)
    return EmitLibCall();

  uint64_t Count = CountNode->getZExtValue();
  if (Count >= 32)
    return EmitLibCall();
  if (Count == 0)
    return Value;
  if (Op.getOpcode() == ISD::SHL && Count == 1)
    return DAG.getNode(ISD::ADD, DL, MVT::i32, Value, Value);
  if (Op.getOpcode() == ISD::SRL && Count == 1)
    return DAG.getNode(AVMISD::LSR32_1, DL, MVT::i32, Value);
  if (Op.getOpcode() == ISD::SRA && Count == 1)
    return DAG.getNode(AVMISD::SRA32_1, DL, MVT::i32, Value);

  auto [Low, High] = getI32Words(Value, DL, DAG);
  if (Value.getOpcode() != ISD::BUILD_PAIR &&
      Value.getOpcode() != AVMISD::BUILD_HI16 &&
      Value.getOpcode() != ISD::ZERO_EXTEND &&
      Value.getOpcode() != ISD::SIGN_EXTEND &&
      Value.getOpcode() != ISD::ANY_EXTEND) {
    // An opaque full-width input benefits from the native pair carrier. Keep
    // exposed or known-zero words scalar, as needed for aggregate packing.
    KnownBits Known = DAG.computeKnownBits(Value);
    if (!Known.Zero.getLoBits(16).isAllOnes() &&
        !Known.Zero.getHiBits(16).isAllOnes()) {
      if (Count == 16 && Op.getOpcode() == ISD::SRA)
        return DAG.getNode(AVMISD::SRA32_16, DL, MVT::i32, Value);
      High = DAG.getNode(ISD::TRUNCATE, DL, MVT::i16,
                         DAG.getNode(AVMISD::SRL32_16, DL, MVT::i32, Value));
    }
  }

  auto ShiftCount = [&](uint64_t Amount) {
    return DAG.getConstant(Amount, DL, MVT::i16);
  };

  if (Count < 16) {
    SDValue NewLow;
    SDValue NewHigh;
    if (Op.getOpcode() == ISD::SHL) {
      NewLow = DAG.getNode(ISD::SHL, DL, MVT::i16, Low, ShiftCount(Count));
      SDValue Carry =
          DAG.getNode(ISD::SRL, DL, MVT::i16, Low, ShiftCount(16 - Count));
      SDValue Shifted =
          DAG.getNode(ISD::SHL, DL, MVT::i16, High, ShiftCount(Count));
      NewHigh = DAG.getNode(ISD::OR, DL, MVT::i16, Shifted, Carry);
    } else {
      SDValue ShiftedLow =
          DAG.getNode(ISD::SRL, DL, MVT::i16, Low, ShiftCount(Count));
      SDValue Carry =
          DAG.getNode(ISD::SHL, DL, MVT::i16, High, ShiftCount(16 - Count));
      NewLow = DAG.getNode(ISD::OR, DL, MVT::i16, ShiftedLow, Carry);
      NewHigh = DAG.getNode(Op.getOpcode() == ISD::SRL ? ISD::SRL : ISD::SRA,
                            DL, MVT::i16, High, ShiftCount(Count));
    }
    return BuildI32FromWords(NewLow, NewHigh, DL, DAG);
  }

  if (Count == 16) {
    SDValue Zero = DAG.getConstant(0, DL, MVT::i16);
    if (Op.getOpcode() == ISD::SHL)
      return BuildI32FromWords(Zero, Low, DL, DAG);
    SDValue Sign = Op.getOpcode() == ISD::SRA
                       ? DAG.getNode(ISD::SRA, DL, MVT::i16, High, ShiftCount(15))
                       : Zero;
    return BuildI32FromWords(High, Sign, DL, DAG);
  }

  uint64_t WordCount = Count - 16;
  if (Op.getOpcode() == ISD::SHL) {
    SDValue NewHigh =
        DAG.getNode(ISD::SHL, DL, MVT::i16, Low, ShiftCount(WordCount));
    return BuildI32FromWords(DAG.getConstant(0, DL, MVT::i16), NewHigh, DL, DAG);
  }
  if (Op.getOpcode() == ISD::SRL) {
    SDValue NewLow =
        DAG.getNode(ISD::SRL, DL, MVT::i16, High, ShiftCount(WordCount));
    return BuildI32FromWords(NewLow, DAG.getConstant(0, DL, MVT::i16), DL, DAG);
  }

  SDValue NewLow =
      DAG.getNode(ISD::SRA, DL, MVT::i16, High, ShiftCount(WordCount));
  SDValue Sign = DAG.getNode(ISD::SRA, DL, MVT::i16, High, ShiftCount(15));
  return BuildI32FromWords(NewLow, Sign, DL, DAG);
}

void AVMTargetLowering::computeKnownBitsForTargetNode(
    SDValue Op, KnownBits &Known, const APInt &DemandedElts,
    const SelectionDAG &DAG, unsigned Depth) const {
  KnownBits Count = KnownBits::makeConstant(APInt(32, 16));
  switch (Op.getOpcode()) {
  case AVMISD::BOOL:
    Known.resetAll();
    Known.Zero.setBitsFrom(1);
    return;
  case AVMISD::BUILD_HI16:
    Known = DAG.computeKnownBits(Op.getOperand(0), Depth + 1).zext(32);
    Known = KnownBits::shl(Known, Count);
    return;
  case AVMISD::SHL32_16:
    Known = KnownBits::shl(DAG.computeKnownBits(Op.getOperand(0), Depth + 1),
                           Count);
    return;
  case AVMISD::SRL32_16:
    Known = KnownBits::lshr(DAG.computeKnownBits(Op.getOperand(0), Depth + 1),
                            Count);
    return;
  case AVMISD::SRA32_16:
    Known = KnownBits::ashr(DAG.computeKnownBits(Op.getOperand(0), Depth + 1),
                            Count);
    return;
  default:
    TargetLowering::computeKnownBitsForTargetNode(Op, Known, DemandedElts, DAG,
                                                 Depth);
  }
}

SDValue AVMTargetLowering::PerformDAGCombine(SDNode *N,
                                             DAGCombinerInfo &DCI) const {
  if (N->getOpcode() == AVMISD::BR_CC || N->getOpcode() == AVMISD::CMOV ||
      N->getOpcode() == AVMISD::CSET) {
    // Operand legalization can expose classification's CSET only after the
    // branch/select was lowered. Recover its native flags here as well.
    unsigned GlueIndex = N->getNumOperands() - 1, CCIndex = GlueIndex - 1;
    SDValue Glue = N->getOperand(GlueIndex);
    unsigned CC = N->getConstantOperandVal(CCIndex);
    if ((CC == AVMCC::EQ || CC == AVMCC::NE) && Glue.hasOneUse() &&
        (Glue.getOpcode() == AVMISD::TST8 ||
         Glue.getOpcode() == AVMISD::TST16)) {
      SDValue Bool = Glue.getOperand(0), Masked = Bool;
      if (Bool.getOpcode() == ISD::AND &&
          isa<ConstantSDNode>(Bool.getOperand(1)) &&
          Bool.getConstantOperandVal(1) == 1)
        Bool = Bool.getOperand(0);
      if (Bool.getOpcode() == AVMISD::CSET && Bool.hasOneUse() &&
          Masked.hasOneUse()) {
        unsigned Predicate = Bool.getConstantOperandVal(0);
        if (CC == AVMCC::EQ)
          Predicate ^= 1;
        SmallVector<SDValue, 5> Ops(N->op_begin(), N->op_end());
        Ops[CCIndex] = DCI.DAG.getTargetConstant(Predicate, SDLoc(N), MVT::i16);
        Ops[GlueIndex] = Bool.getOperand(1);
        return DCI.DAG.getNode(N->getOpcode(), SDLoc(N), N->getVTList(), Ops);
      }
    }
  }
  if (N->getOpcode() == ISD::UMIN && N->getValueType(0) == MVT::i32) {
    SDValue Sum = N->getOperand(0), Limit = N->getOperand(1);
    if (isa<ConstantSDNode>(Sum))
      std::swap(Sum, Limit);
    const auto *Max = dyn_cast<ConstantSDNode>(Limit);
    if (Max && Max->getZExtValue() == UINT16_MAX &&
        Sum.getOpcode() == ISD::ADD && Sum.hasOneUse()) {
      SelectionDAG &DAG = DCI.DAG;
      SDValue LHS = Sum.getOperand(0), RHS = Sum.getOperand(1);
      // Recognize a clamped widening add before min/max expansion hides it
      // behind a wide compare and select. Ranges, rather than just explicit
      // extensions, also cover masked operands and zero-extending loads.
      // Two unsigned words cannot overflow i32, so this preserves all bits
      // even when callers retain the clamped result as an i32 value.
      if (DAG.computeKnownBits(LHS).getMaxValue().ule(UINT16_MAX) &&
          DAG.computeKnownBits(RHS).getMaxValue().ule(UINT16_MAX)) {
        SDLoc DL(N);
        SDValue Sat = DAG.getNode(
            ISD::UADDSAT, DL, MVT::i16,
            DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, LHS),
            DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, RHS));
        return DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i32, Sat);
      }
    }
    return SDValue();
  }
  if (N->getOpcode() == ISD::BUILD_PAIR && N->getValueType(0) == MVT::i32) {
    SDValue Pair = BuildI32FromWords(N->getOperand(0), N->getOperand(1),
                                    SDLoc(N), DCI.DAG);
    return Pair == SDValue(N, 0) ? SDValue() : Pair;
  }
  if (N->getOpcode() == ISD::EXTRACT_ELEMENT &&
      N->getValueType(0) == MVT::i16 &&
      N->getOperand(0).getValueType() == MVT::i32) {
    SDValue Word = getI32Word(N->getOperand(0), N->getConstantOperandVal(1),
                              SDLoc(N), DCI.DAG);
    return Word == SDValue(N, 0) ? SDValue() : Word;
  }
  if (N->getOpcode() == ISD::TRUNCATE && N->getValueType(0) == MVT::i16 &&
      N->getOperand(0).getValueType() == MVT::i32) {
    SDValue Word = getI32Word(N->getOperand(0), 0, SDLoc(N), DCI.DAG);
    // Keep the generic truncation available for demanded-bits and memory
    // combines unless this actually exposes a native word or constant.
    if (Word.getOpcode() == ISD::EXTRACT_ELEMENT &&
        Word.getOperand(0) == N->getOperand(0))
      return SDValue();
    return Word == SDValue(N, 0) ? SDValue() : Word;
  }

  if (auto *Load = dyn_cast<LoadSDNode>(N)) {
    // Every unsigned byte load zeroes the rest of its destination word. Make
    // that choice explicit before instruction selection so all users (including
    // outgoing ABI extensions) can use the known upper bits. EXTLOAD alone
    // leaves those bits unspecified, hiding the instruction's actual semantics.
    if (Load->getMemoryVT() == MVT::i8 &&
        Load->getValueType(0) == MVT::i16 &&
        Load->getExtensionType() == ISD::EXTLOAD &&
        (Load->getAddressSpace() == 0 || Load->getAddressSpace() == 1)) {
      SDValue Result = DCI.DAG.getExtLoad(ISD::ZEXTLOAD, SDLoc(N), MVT::i16,
                                          Load->getChain(), Load->getBasePtr(),
                                          MVT::i8, Load->getMemOperand());
      if (!Load->isUnindexed())
        Result = DCI.DAG.getIndexedLoad(Result, SDLoc(N), Load->getBasePtr(),
                                        Load->getOffset(),
                                        Load->getAddressingMode());
      return Result;
    }
    return SDValue();
  }

  if (N->getOpcode() == ISD::MUL && N->getValueType(0) == MVT::i32) {
    // Program-pointer indexing uses i32 offsets even for byte indices. Keep
    // the multiplication native when the entire unsigned product fits in a
    // word, then restore the wide offset with a zero high word. Compute the
    // bound in 64 bits so it cannot wrap for unrestricted i32 operands.
    SelectionDAG &DAG = DCI.DAG;
    SDLoc DL(N);
    SDValue LHS = N->getOperand(0), RHS = N->getOperand(1);
    KnownBits Left = DAG.computeKnownBits(LHS);
    KnownBits Right = DAG.computeKnownBits(RHS);
    uint64_t MaxLHS = Left.getMaxValue().getZExtValue();
    uint64_t MaxRHS = Right.getMaxValue().getZExtValue();
    auto LowProduct = [&]() {
      return DAG.getNode(ISD::MUL, DL, MVT::i16,
                         DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, LHS),
                         DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, RHS));
    };
    if (MaxLHS * MaxRHS <= UINT16_MAX) {
      return DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i32, LowProduct());
    }

    // KnownBits alone cannot express that unknown high bits are copies of
    // the sign bit. Intersect its bounds with ComputeNumSignBits so signed
    // extensions and sign-extended loads retain their narrow value ranges.
    auto SignedBounds = [&](SDValue Value, const KnownBits &Known) {
      int64_t Magnitude = int64_t(1) << (32 - DAG.ComputeNumSignBits(Value));
      return std::pair(
          std::max(-Magnitude, Known.getSignedMinValue().getSExtValue()),
          std::min(Magnitude - 1, Known.getSignedMaxValue().getSExtValue()));
    };
    auto [MinLeft, MaxLeft] = SignedBounds(LHS, Left);
    auto [MinRight, MaxRight] = SignedBounds(RHS, Right);
    // All four signed i32 endpoint products fit in int64_t.
    int64_t Products[] = {MinLeft * MinRight, MinLeft * MaxRight,
                          MaxLeft * MinRight, MaxLeft * MaxRight};
    auto [MinProduct, MaxProduct] =
        std::minmax_element(std::begin(Products), std::end(Products));
    if (*MinProduct >= INT16_MIN && *MaxProduct <= INT16_MAX)
      return DAG.getNode(ISD::SIGN_EXTEND, DL, MVT::i32, LowProduct());

    // A product in [-65536, 65535] has only a sign in its high word, even
    // when its low word overflows signed i16 (e.g. -128 * 257 = -32896).
    // A strictly positive factor preserves the other operand's sign. Zero
    // must be excluded: negative * zero needs a zero high word as well.
    if (*MinProduct >= -65536 && *MaxProduct <= UINT16_MAX &&
        (MinLeft > 0 || MinRight > 0)) {
      SDValue Signed = MinRight > 0 ? LHS : RHS;
      int64_t MinSigned = MinRight > 0 ? MinLeft : MinRight;
      int64_t MaxSigned = MinRight > 0 ? MaxLeft : MaxRight;
      unsigned SignWord =
          MinSigned >= INT16_MIN && MaxSigned <= INT16_MAX ? 0 : 1;
      SDValue High = DAG.getNode(ISD::SRA, DL, MVT::i16,
                                 getI32Word(Signed, SignWord, DL, DAG),
                                 DAG.getConstant(15, DL, MVT::i16));
      return BuildI32FromWords(LowProduct(), High, DL, DAG);
    }
    return SDValue();
  }

  if (N->getValueType(0) != MVT::i16)
    return SDValue();

  if (N->getOpcode() == ISD::MUL) {
    SDValue Value = N->getOperand(0);
    const auto *Constant = dyn_cast<ConstantSDNode>(N->getOperand(1));
    if (!Constant) {
      Constant = dyn_cast<ConstantSDNode>(Value);
      Value = N->getOperand(1);
    }
    if (Constant && Constant->getZExtValue() == 5) {
      // MULU8W/MULS8W can beat the shift-and-add sequence for a byte value.
      // The generic constant-multiply hook has no access to this operand.
      KnownBits Known = DCI.DAG.computeKnownBits(Value);
      bool UnsignedByte =
          (Known.Zero & APInt(16, 0xff00)) == APInt(16, 0xff00);
      bool SignedByte = DCI.DAG.ComputeNumSignBits(Value) >= 9;
      if (!UnsignedByte && !SignedByte) {
        SDValue Shift = DCI.DAG.getNode(
            ISD::SHL, SDLoc(N), MVT::i16, Value,
            DCI.DAG.getConstant(2, SDLoc(N), MVT::i16));
        return DCI.DAG.getNode(ISD::ADD, SDLoc(N), MVT::i16, Shift, Value);
      }
    }
  }

  SDValue Product = SDValue(N, 0);
  SDValue Dividend;
  if (N->getOpcode() == ISD::SUB) {
    Dividend = N->getOperand(0);
    Product = N->getOperand(1);
    if (Product.getOpcode() != ISD::MUL || !Product.getNode()->hasOneUse())
      return SDValue();
  } else if (N->getOpcode() != ISD::MUL) {
    return SDValue();
  }

  for (unsigned DivOperand = 0; DivOperand != 2; ++DivOperand) {
    SDValue Div = Product.getOperand(DivOperand);
    if ((Div.getOpcode() != ISD::UDIV && Div.getOpcode() != ISD::SDIV) ||
        !Div.getNode()->hasOneUse() ||
        Product.getOperand(1 - DivOperand) != Div.getOperand(1))
      continue;

    SDValue DivDividend = Div.getOperand(0);
    if (Dividend && Dividend != DivDividend)
      continue;

    unsigned RemOpcode = Div.getOpcode() == ISD::UDIV ? ISD::UREM : ISD::SREM;
    SDValue Rem = DCI.DAG.getNode(RemOpcode, SDLoc(N), MVT::i16, DivDividend,
                                  Div.getOperand(1));
    if (Dividend)
      return Rem;

    // Preserve the product's value while exposing a native remainder to the
    // surrounding subtraction. This also handles legalized narrow remainders
    // whose later reassociation obscures the original SUB dividend.
    return DCI.DAG.getNode(ISD::SUB, SDLoc(N), MVT::i16, DivDividend, Rem);
  }
  return SDValue();
}

static SDValue lowerI32ConstantMultiply(SDValue Op, SelectionDAG &DAG) {
  SDValue Value = Op.getOperand(0);
  const auto *Constant = dyn_cast<ConstantSDNode>(Op.getOperand(1));
  if (!Constant) {
    Constant = dyn_cast<ConstantSDNode>(Value);
    Value = Op.getOperand(1);
  }
  if (!Constant)
    return SDValue();

  // Work modulo 2^32, including negative constants and INT32_MIN. Preserve
  // native narrow products in the DAG combine; this is the fallback before
  // choosing either of the full-product runtime helpers.
  int64_t Signed = Constant->getSExtValue();
  uint64_t Magnitude = Signed < 0 ? -Signed : Signed;
  if (!Magnitude)
    return DAG.getConstant(0, SDLoc(Op), MVT::i32);
  bool IsPower = isPowerOf2_64(Magnitude);
  bool IsAdd = !IsPower && isPowerOf2_64(Magnitude - 1);
  if (!IsPower && !IsAdd && !isPowerOf2_64(Magnitude + 1))
    return SDValue();
  unsigned Shift = Log2_64(IsPower ? Magnitude
                                 : IsAdd ? Magnitude - 1 : Magnitude + 1);
  if (Shift >= 32)
    return SDValue();

  using AVM::AVMCostKind;
  auto Cost = AVM::getFixedCycles;
  bool OptSize = DAG.getMachineFunction().getFunction().hasOptSize();
  auto WordShiftCost = [&](unsigned Count) {
    if (!Count)
      return 0U;
    return !OptSize && Count <= 3
               ? Count * Cost(AVMCostKind::AddUpper)
               : AVM::getShiftCycles(AVMCostKind::Lsl16I, Count);
  };
  // Match LowerI32Shift's word shifts, carry merge, and word transfers.
  // Charge copies conservatively, since allocation may place words outside
  // the upper registers. Repeated ADD32s often win for small shift counts.
  unsigned NativeShiftCost = 0;
  if (Shift && Shift < 16)
    NativeShiftCost = 2 * WordShiftCost(Shift) +
                      AVM::getShiftCycles(AVMCostKind::Lsr16I, 16 - Shift) +
                      Cost(AVMCostKind::OrFull) +
                      2 * Cost(AVMCostKind::MovFull);
  else if (Shift >= 16)
    NativeShiftCost = WordShiftCost(Shift - 16) +
                      2 * Cost(AVMCostKind::MovFull) +
                      Cost(AVMCostKind::ClrUpper);
  unsigned DoubleCost = Shift * Cost(AVMCostKind::Add32);
  bool UseDoubles = DoubleCost <= NativeShiftCost;
  unsigned Cycles = std::min(DoubleCost, NativeShiftCost);
  if (!IsPower)
    Cycles += Cost(AVMCostKind::Mov32Full) +
              Cost(IsAdd ? AVMCostKind::Add32 : AVMCostKind::Sub32);
  // -(2^N - 1) is x - (x << N); the other negative forms need a negation.
  bool Negate = Signed < 0 && (IsPower || IsAdd);
  if (Negate)
    Cycles += 2 * Cost(AVMCostKind::ClrUpper) + Cost(AVMCostKind::Sub32);

  // Use only the call/return overhead as a budget, excluding the helper body
  // and argument setup. This deliberately limits code growth and live ranges.
  if (Cycles > Cost(AVMCostKind::Call8) + Cost(AVMCostKind::Ret))
    return SDValue();

  SDLoc DL(Op);
  SDValue Scaled = Value;
  if (UseDoubles) {
    for (unsigned I = 0; I != Shift; ++I)
      Scaled = DAG.getNode(ISD::ADD, DL, MVT::i32, Scaled, Scaled);
  } else {
    Scaled = DAG.getNode(ISD::SHL, DL, MVT::i32, Value,
                         DAG.getConstant(Shift, DL, MVT::i16));
  }
  if (!IsPower) {
    if (IsAdd)
      Scaled = DAG.getNode(ISD::ADD, DL, MVT::i32, Scaled, Value);
    else if (Signed < 0)
      Scaled = DAG.getNode(ISD::SUB, DL, MVT::i32, Value, Scaled);
    else
      Scaled = DAG.getNode(ISD::SUB, DL, MVT::i32, Scaled, Value);
  }
  return Negate ? DAG.getNode(ISD::SUB, DL, MVT::i32,
                              DAG.getConstant(0, DL, MVT::i32), Scaled)
                : Scaled;
}

SDValue AVMTargetLowering::LowerI32Multiply(SDValue Op,
                                         SelectionDAG &DAG) const {
  if (SDValue Scaled = lowerI32ConstantMultiply(Op, DAG))
    return Scaled;

  SDLoc DL(Op);
  SDValue LHS = Op.getOperand(0), RHS = Op.getOperand(1);
  // Select by the operands' value ranges, including masks and extending
  // loads. The full product can exceed a word even though both inputs fit.
  bool IsUnsigned = DAG.computeKnownBits(LHS).getMaxValue().ule(UINT16_MAX) &&
                    DAG.computeKnownBits(RHS).getMaxValue().ule(UINT16_MAX);
  bool IsSigned = DAG.ComputeNumSignBits(LHS) >= 17 &&
                  DAG.ComputeNumSignBits(RHS) >= 17;
  if (IsUnsigned || IsSigned)
    return LowerI16FullMultiply(
        getI32Word(LHS, 0, DL, DAG), getI32Word(RHS, 0, DL, DAG),
        !IsUnsigned, DL, DAG);

  MakeLibCallOptions CallOptions;
  SDValue Ops[] = {LHS, RHS};
  return makeLibCall(DAG, RTLIB::MUL_I32, MVT::i32, Ops, CallOptions, DL).first;
}

SDValue AVMTargetLowering::LowerI16FullMultiply(SDValue LHS, SDValue RHS,
                                                bool IsSigned, const SDLoc &DL,
                                                SelectionDAG &DAG) const {
  assert(LHS.getValueType() == MVT::i16 && RHS.getValueType() == MVT::i16 &&
         "expected i16 multiply operands");

  ArgListTy Args;
  for (SDValue Arg : {LHS, RHS}) {
    ArgListEntry Entry(Arg, Type::getInt16Ty(*DAG.getContext()));
    Entry.IsSExt = IsSigned;
    Entry.IsZExt = !IsSigned;
    Args.push_back(Entry);
  }
  SDValue Callee = DAG.getExternalSymbol(
      IsSigned ? "__avm_mulhisi3" : "__avm_umulhisi3",
      getPointerTy(DAG.getDataLayout()));
  CallLoweringInfo CLI(DAG);
  CLI.setDebugLoc(DL)
      .setChain(DAG.getEntryNode())
      .setLibCallee(CallingConv::C, Type::getInt32Ty(*DAG.getContext()), Callee,
                   std::move(Args));
  return LowerCallTo(CLI).first;
}

SDValue AVMTargetLowering::LowerMULH(SDValue Op, SelectionDAG &DAG) const {
  assert((Op.getOpcode() == ISD::MULHU || Op.getOpcode() == ISD::MULHS) &&
         Op.getValueType() == MVT::i16 &&
         "unexpected AVM multiply-high operation");

  SDLoc DL(Op);
  bool IsSigned = Op.getOpcode() == ISD::MULHS;

  SDValue Product = LowerI16FullMultiply(Op.getOperand(0), Op.getOperand(1),
                                         IsSigned, DL, DAG);

  SDValue High = DAG.getNode(AVMISD::SRL32_16, DL, MVT::i32, Product);

  return DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, High);
}

SDValue AVMTargetLowering::LowerMUL_LOHI(SDValue Op, SelectionDAG &DAG) const {
  assert(
      (Op.getOpcode() == ISD::UMUL_LOHI || Op.getOpcode() == ISD::SMUL_LOHI) &&
      Op.getValueType() == MVT::i16 &&
      "unexpected AVM multiply low/high operation");

  SDLoc DL(Op);
  bool IsSigned = Op.getOpcode() == ISD::SMUL_LOHI;

  SDValue Product = LowerI16FullMultiply(Op.getOperand(0), Op.getOperand(1),
                                         IsSigned, DL, DAG);

  SDValue Low = DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, Product);

  SDValue ShiftedHigh = DAG.getNode(AVMISD::SRL32_16, DL, MVT::i32, Product);
  SDValue High = DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, ShiftedHigh);

  return DAG.getMergeValues({Low, High}, DL);
}

void AVMTargetLowering::ReplaceNodeResults(
    SDNode *N, SmallVectorImpl<SDValue> &Results, SelectionDAG &DAG) const {
  if (N->getOpcode() == ISD::CTTZ ||
      N->getOpcode() == ISD::CTTZ_ZERO_UNDEF ||
      N->getOpcode() == ISD::CTLZ ||
      N->getOpcode() == ISD::CTLZ_ZERO_UNDEF) {
    SDLoc DL(N);
    bool IsTrailing = N->getOpcode() == ISD::CTTZ ||
                      N->getOpcode() == ISD::CTTZ_ZERO_UNDEF;
    SDValue Value = N->getOperand(0);
    SDValue Low = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i32, Value,
                              DAG.getConstant(0, DL, MVT::i32));
    SDValue High = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i32, Value,
                               DAG.getConstant(1, DL, MVT::i32));
    unsigned CountOpcode = IsTrailing ? ISD::CTTZ : ISD::CTLZ;
    SDValue LowCount = DAG.getNode(CountOpcode, DL, MVT::i32, Low);
    SDValue HighCount = DAG.getNode(CountOpcode, DL, MVT::i32, High);
    SDValue HighIsZero = DAG.getSetCC(
        DL, MVT::i1, High, DAG.getConstant(0, DL, MVT::i32), ISD::SETEQ);
    SDValue LowIsZero = DAG.getSetCC(
        DL, MVT::i1, Low, DAG.getConstant(0, DL, MVT::i32), ISD::SETEQ);
    SDValue AddWord = IsTrailing ? HighCount : LowCount;
    SDValue CountWithWord = DAG.getNode(
        ISD::ADD, DL, MVT::i32, AddWord,
        DAG.getConstant(32, DL, MVT::i32));
    SDValue UseOtherWord = IsTrailing ? LowIsZero : HighIsZero;
    SDValue PrimaryCount = IsTrailing ? LowCount : HighCount;
    SDValue Count = DAG.getNode(ISD::SELECT, DL, MVT::i32, UseOtherWord,
                                CountWithWord, PrimaryCount);
    Results.push_back(DAG.getZExtOrTrunc(Count, DL, MVT::i64));
    return;
  }

  assert((N->getOpcode() == ISD::UDIVREM ||
          N->getOpcode() == ISD::SDIVREM) &&
         N->getValueType(0) == MVT::i64 && "unexpected custom legalization");
  bool IsSigned = N->getOpcode() == ISD::SDIVREM;
  bool HasQuotient = N->hasAnyUseOfValue(0);
  bool HasRemainder = N->hasAnyUseOfValue(1);
  SDLoc DL(N);
  MakeLibCallOptions CallOptions;
  CallOptions.setIsSigned(IsSigned);
  SmallVector<SDValue, 3> Ops(N->op_values());

  // Type legalization also routes standalone i64 DIV/REM through DIVREM.
  // Keep their existing helpers and avoid an unused stack output argument.
  if (!HasQuotient || !HasRemainder) {
    RTLIB::Libcall LC = HasQuotient
                            ? (IsSigned ? RTLIB::SDIV_I64 : RTLIB::UDIV_I64)
                            : (IsSigned ? RTLIB::SREM_I64 : RTLIB::UREM_I64);
    SDValue Value = makeLibCall(DAG, LC, MVT::i64, Ops, CallOptions, DL).first;
    Results.push_back(HasQuotient ? Value : DAG.getUNDEF(MVT::i64));
    Results.push_back(HasRemainder ? Value : DAG.getUNDEF(MVT::i64));
    return;
  }

  SDValue RemainderPtr = DAG.CreateStackTemporary(MVT::i64);
  Ops.push_back(RemainderPtr);
  // Preserve the pointer's ABI type instead of treating it as an integer.
  Type *IntTy = Type::getInt64Ty(*DAG.getContext());
  Type *ArgTypes[] = {IntTy, IntTy,
                      PointerType::getUnqual(*DAG.getContext())};
  CallOptions.setOpsTypeOverrides(ArgTypes);
  RTLIB::Libcall LC = IsSigned ? RTLIB::SDIVREM_I64 : RTLIB::UDIVREM_I64;
  auto [Quotient, Chain] =
      makeLibCall(DAG, LC, MVT::i64, Ops, CallOptions, DL);
  int FI = cast<FrameIndexSDNode>(RemainderPtr)->getIndex();
  SDValue Remainder = DAG.getLoad(
      MVT::i64, DL, Chain, RemainderPtr,
      MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI));
  Results.push_back(Quotient);
  Results.push_back(Remainder);
}

SDValue AVMTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::STRICT_FADD:
  case ISD::STRICT_FSUB:
  case ISD::STRICT_FMUL:
  case ISD::STRICT_FDIV:
  case ISD::STRICT_FSQRT:
  case ISD::STRICT_SINT_TO_FP:
  case ISD::STRICT_UINT_TO_FP:
  case ISD::STRICT_FP_TO_SINT:
  case ISD::STRICT_FP_TO_UINT:
  case ISD::STRICT_FSETCC:
  case ISD::STRICT_FSETCCS: {
    // AVM supports only nearest-even rounding and no observable exceptions.
    // Dynamic rounding is therefore nearest-even too. Preserve the incoming
    // chain while using the same instructions as ordinary FP operations.
    unsigned Opcode;
    switch (Op.getOpcode()) {
    case ISD::STRICT_FADD: Opcode = ISD::FADD; break;
    case ISD::STRICT_FSUB: Opcode = ISD::FSUB; break;
    case ISD::STRICT_FMUL: Opcode = ISD::FMUL; break;
    case ISD::STRICT_FDIV: Opcode = ISD::FDIV; break;
    case ISD::STRICT_FSQRT: Opcode = ISD::FSQRT; break;
    case ISD::STRICT_SINT_TO_FP: Opcode = ISD::SINT_TO_FP; break;
    case ISD::STRICT_UINT_TO_FP: Opcode = ISD::UINT_TO_FP; break;
    case ISD::STRICT_FP_TO_SINT: Opcode = ISD::FP_TO_SINT; break;
    case ISD::STRICT_FP_TO_UINT: Opcode = ISD::FP_TO_UINT; break;
    default: Opcode = ISD::SETCC; break;
    }
    SmallVector<SDValue, 3> Operands;
    for (unsigned I = 1; I < Op.getNumOperands(); ++I)
      Operands.push_back(Op.getOperand(I));
    SDValue Result = DAG.getNode(Opcode, SDLoc(Op), Op.getValueType(), Operands);
    return DAG.getMergeValues({Result, Op.getOperand(0)}, SDLoc(Op));
  }
  case ISD::UADDSAT: {
    SDLoc DL(Op);
    SDValue LHS = Op.getOperand(0), RHS = Op.getOperand(1);
    // Unsigned overflow is exactly (LHS + RHS) mod 2^16 < LHS. Keep
    // the comparison as glue so no intermediate boolean is materialized.
    SDValue Sum = DAG.getNode(ISD::ADD, DL, MVT::i16, LHS, RHS);
    auto [TargetCC, Glue] = getAVMCompare(Sum, LHS, ISD::SETUGE, DL, DAG);
    return DAG.getNode(AVMISD::CMOV, DL, MVT::i16, Sum,
                       DAG.getAllOnesConstant(DL, MVT::i16), TargetCC, Glue);
  }
  case ISD::EXTRACT_ELEMENT:
    return getI32Word(Op.getOperand(0), Op.getConstantOperandVal(1), SDLoc(Op),
                       DAG);
  case ISD::CTTZ:
  case ISD::CTTZ_ZERO_UNDEF: {
    SDLoc DL(Op);
    SDValue Low = getI32Word(Op.getOperand(0), 0, DL, DAG);
    SDValue High = getI32Word(Op.getOperand(0), 1, DL, DAG);
    SDValue LowCount = DAG.getNode(ISD::CTTZ, DL, MVT::i16, Low);
    SDValue HighCount = DAG.getNode(ISD::CTTZ, DL, MVT::i16, High);
    SDValue HighCountPlusWord = DAG.getNode(
        ISD::ADD, DL, MVT::i16, HighCount,
        DAG.getConstant(16, DL, MVT::i16));
    SDValue LowIsZero = DAG.getSetCC(
        DL, MVT::i1, Low, DAG.getConstant(0, DL, MVT::i16), ISD::SETEQ);
    SDValue Count = DAG.getNode(ISD::SELECT, DL, MVT::i16, LowIsZero,
                                HighCountPlusWord, LowCount);
    return DAG.getNode(ISD::ZERO_EXTEND, DL, Op.getValueType(), Count);
  }
  case ISD::CTLZ:
  case ISD::CTLZ_ZERO_UNDEF: {
    SDLoc DL(Op);
    SDValue Low = getI32Word(Op.getOperand(0), 0, DL, DAG);
    SDValue High = getI32Word(Op.getOperand(0), 1, DL, DAG);
    SDValue LowCount = DAG.getNode(ISD::CTLZ, DL, MVT::i16, Low);
    SDValue HighCount = DAG.getNode(ISD::CTLZ, DL, MVT::i16, High);
    SDValue LowCountPlusWord = DAG.getNode(
        ISD::ADD, DL, MVT::i16, LowCount,
        DAG.getConstant(16, DL, MVT::i16));
    SDValue HighIsZero = DAG.getSetCC(
        DL, MVT::i1, High, DAG.getConstant(0, DL, MVT::i16), ISD::SETEQ);
    SDValue Count = DAG.getNode(ISD::SELECT, DL, MVT::i16, HighIsZero,
                                LowCountPlusWord, HighCount);
    return DAG.getNode(ISD::ZERO_EXTEND, DL, Op.getValueType(), Count);
  }
  case ISD::INTRINSIC_VOID: {
    const auto *ID = cast<ConstantSDNode>(Op.getOperand(1));
    SDLoc DL(Op);

    SmallVector<SDValue, 8> Ops(Op->op_begin(), Op->op_end());

    auto PromoteUnsignedByte = [&](unsigned OperandIndex) {
      assert(Ops[OperandIndex].getValueType() == MVT::i8 &&
            "expected an i8 intrinsic operand");
      Ops[OperandIndex] =
          DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i16, Ops[OperandIndex]);
    };

    switch (ID->getZExtValue()) {
    case Intrinsic::avm_debug_putc:
    case Intrinsic::avm_draw_filled_rect_white:
    case Intrinsic::avm_draw_filled_rect_black:
    case Intrinsic::avm_set_text_mode:
      for (unsigned I = 2; I != Ops.size(); ++I)
        if (Ops[I].getValueType() == MVT::i8)
          Ops[I] = DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i16, Ops[I]);
      break;
    default:
      report_fatal_error("unexpected AVM intrinsic with an i8 operand");
    }

    return DAG.getNode(ISD::INTRINSIC_VOID, DL, MVT::Other, Ops);
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
  case ISD::MUL:
    return LowerI32Multiply(Op, DAG);
  case ISD::MULHU:
  case ISD::MULHS:
    return LowerMULH(Op, DAG);
  case ISD::UMUL_LOHI:
  case ISD::SMUL_LOHI:
    return LowerMUL_LOHI(Op, DAG);
  case ISD::ATOMIC_FENCE:
    return Op.getOperand(0);
  case ISD::ADDRSPACECAST:
    report_fatal_error(
        "AVM does not support casts between address spaces 0 and 1");
  case ISD::SHL:
  case ISD::SRL:
  case ISD::SRA:
    return LowerI32Shift(Op, DAG);
  case ISD::ROTL:
  case ISD::ROTR:
    return LowerI32Rotate(Op, DAG);
  case ISD::AND:
  case ISD::OR:
    return LowerI32Bitwise(Op, DAG);
  case ISD::SELECT:
    return LowerSelect(Op, DAG);
  case ISD::SELECT_CC:
    return LowerSelectCC(Op, DAG);
  case ISD::SETCC:
    return LowerSetCC(Op, DAG);
  case ISD::INTRINSIC_WO_CHAIN: {
    const char *Name;
    switch (cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue()) {
    case Intrinsic::avm_sinf: Name = "sinf"; break;
    case Intrinsic::avm_cosf: Name = "cosf"; break;
    case Intrinsic::avm_atan2f: Name = "atan2f"; break;
    case Intrinsic::avm_tanf: Name = "tanf"; break;
    case Intrinsic::avm_expf: Name = "expf"; break;
    case Intrinsic::avm_logf: Name = "logf"; break;
    case Intrinsic::avm_log2f: Name = "log2f"; break;
    case Intrinsic::avm_log10f: Name = "log10f"; break;
    case Intrinsic::avm_powf: Name = "powf"; break;
    case Intrinsic::avm_hypotf: Name = "hypotf"; break;
    case Intrinsic::avm_fmodf:
      return DAG.getNode(ISD::FREM, SDLoc(Op), MVT::f32, Op.getOperand(1), Op.getOperand(2));
    default: return Op;
    }
    ArgListTy Args;
    for (unsigned I = 1; I != Op.getNumOperands(); ++I)
      Args.emplace_back(Op.getOperand(I), Type::getFloatTy(*DAG.getContext()));
    SDValue Callee = DAG.getExternalSymbol(Name, getPointerTy(DAG.getDataLayout()));
    CallLoweringInfo CLI(DAG);
    CLI.setDebugLoc(SDLoc(Op))
        .setChain(DAG.getEntryNode())
        .setLibCallee(CallingConv::C, Type::getFloatTy(*DAG.getContext()), Callee,
                      std::move(Args));
    return LowerCallTo(CLI).first;
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

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const Function &Caller = MF.getFunction();
  SDValue Chain = CLI.Chain;
  SmallVector<CCValAssign, 8> ArgLocs;
  CCState CCInfo(CLI.CallConv, CLI.IsVarArg, MF, ArgLocs, *DAG.getContext());
  analyzeAVMArguments(CLI.Outs, CCInfo, CLI.IsVarArg);
  unsigned NumBytes = CCInfo.getStackSize();
  if (NumBytes > UINT16_MAX)
    report_fatal_error(
        "AVM outgoing stack arguments exceed the 16-bit ABI limit");

  // SelectionDAGBuilder has already checked tail position and return
  // attributes. Restrict sibling calls to the scalar C ABI and compare its
  // actual return locations, including legalized narrow/pointer results.
  CLI.IsTailCall &= Caller.getCallingConv() == CallingConv::C &&
                    !CLI.IsVarArg && !Caller.isVarArg() && NumBytes == 0 &&
                    !Caller.hasStructRetAttr() && CLI.RetTy &&
                    Caller.getReturnType() == CLI.RetTy &&
                    (CLI.RetTy->isVoidTy() || CLI.RetTy->isSingleValueType());
  for (const auto &Out : CLI.Outs)
    if (Out.Flags.isByVal() || Out.Flags.isSRet() || Out.Flags.isInAlloca() ||
        Out.Flags.isPreallocated())
      CLI.IsTailCall = false;
  for (const auto &VA : ArgLocs)
    if (!VA.isRegLoc() || VA.getLocInfo() == CCValAssign::Indirect)
      CLI.IsTailCall = false;

  if (CLI.IsTailCall) {
    SmallVector<ISD::OutputArg, 4> CallerOuts;
    GetReturnInfo(Caller.getCallingConv(), Caller.getReturnType(),
                  Caller.getAttributes(), CallerOuts, *this,
                  DAG.getDataLayout());
    SmallVector<CCValAssign, 4> CallerLocs, CalleeLocs;
    CCState CallerInfo(CallingConv::C, false, MF, CallerLocs,
                       *DAG.getContext());
    CCState CalleeInfo(CallingConv::C, false, MF, CalleeLocs,
                       *DAG.getContext());
    CLI.IsTailCall = analyzeAVMReturns(CallerOuts, CallerInfo) &&
                     analyzeAVMReturns(CLI.Ins, CalleeInfo) &&
                     CallerLocs.size() == CalleeLocs.size();
    if (CLI.IsTailCall)
      for (unsigned I = 0; I != CallerLocs.size(); ++I) {
        const auto &A = CallerLocs[I];
        const auto &B = CalleeLocs[I];
        if (A.getLocReg() != B.getLocReg() || A.getLocVT() != B.getLocVT() ||
            A.getValVT() != B.getValVT() || A.getLocInfo() != B.getLocInfo())
          CLI.IsTailCall = false;
      }
  }

  MCPhysReg TailTargetReg = 0;
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  bool IsDirect = isa<GlobalAddressSDNode>(CLI.Callee) ||
                  isa<ExternalSymbolSDNode>(CLI.Callee);
  if (CLI.IsTailCall && !IsDirect) {
    // A fixed physical copy, glued to the argument copies and transfer,
    // guarantees the target survives both allocation and callee-save pops.
    for (MCPhysReg Pair : AVMArgPairs)
      if (llvm::none_of(ArgLocs, [&](const CCValAssign &VA) {
            return TRI->regsOverlap(Pair, VA.getLocReg());
          })) {
        TailTargetReg = Pair;
        break;
      }
    if (!TailTargetReg)
      CLI.IsTailCall = false;
  }
  if (!CLI.IsTailCall && CLI.CB && CLI.CB->isMustTailCall())
    report_fatal_error("failed to perform AVM tail call elimination on a "
                       "call site marked musttail");

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

  if (!CLI.IsTailCall)
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
    if (TailTargetReg) {
      Chain = DAG.getCopyToReg(Chain, DL, TailTargetReg, Callee, Glue);
      Glue = Chain.getValue(1);
      Callee = DAG.getRegister(TailTargetReg, MVT::i32);
    }
  }

  // Keep the exact callsite size on the machine call through register
  // allocation, frame lowering and machine block duplication.
  SmallVector<SDValue, 10> Ops = {
      Chain, Callee, DAG.getTargetConstant(NumBytes, DL, MVT::i16),
      DAG.getTargetConstant(
          MF.getInfo<AVMMachineFunctionInfo>()->addStackTargets(CLI.CB), DL,
          MVT::i32)};
  for (const auto &[Reg, Value] : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg, Value.getValueType()));
  const uint32_t *Mask =
      TRI->getCallPreservedMask(DAG.getMachineFunction(), CLI.CallConv);
  assert(Mask && "missing AVM call-preserved mask");
  Ops.push_back(DAG.getRegisterMask(Mask));
  if (Glue)
    Ops.push_back(Glue);

  Chain = DAG.getNode(CLI.IsTailCall ? AVMISD::TAIL_CALL : AVMISD::CALL, DL,
                      DAG.getVTList(MVT::Other, MVT::Glue), Ops);
  if (CLI.IsTailCall) {
    MFI.setHasTailCall();
    return Chain;
  }
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
