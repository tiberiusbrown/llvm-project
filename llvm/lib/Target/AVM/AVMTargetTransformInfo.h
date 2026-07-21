//===-- AVMTargetTransformInfo.h - AVM TTI implementation ------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMTARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_AVM_AVMTARGETTRANSFORMINFO_H

#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "AVMTargetMachine.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicsAVM.h"
#include "llvm/Support/MathExtras.h"

namespace llvm {
class AVMTTIImpl final : public BasicTTIImplBase<AVMTTIImpl> {
  using BaseT = BasicTTIImplBase<AVMTTIImpl>;
  using TTI = TargetTransformInfo;
  friend BaseT;

  const AVMSubtarget *ST;
  const AVMTargetLowering *TLI;

  const AVMSubtarget *getST() const { return ST; }
  const AVMTargetLowering *getTLI() const { return TLI; }

  InstructionCost encodedBytes(unsigned Opcode) const {
    return InstructionCost(ST->getInstrInfo()->get(Opcode).getSize());
  }

  static InstructionCost normalizedCycles(unsigned Cycles) {
    return InstructionCost(AVM::normalizeCyclesForTTI(Cycles));
  }

  InstructionCost getHelperArithmeticCost(Type *Ty, unsigned Opcode,
                                          TTI::TargetCostKind CostKind) const {
    unsigned Words = divideCeil(Ty->getScalarSizeInBits(), 16u);
    bool IsShift = Opcode == Instruction::Shl || Opcode == Instruction::LShr ||
                   Opcode == Instruction::AShr;
    unsigned Copies = 2 * Words + (IsShift ? 1 : Words);
    if (CostKind == TTI::TCK_CodeSize)
      return encodedBytes(AVM::CALLF) + Copies * encodedBytes(AVM::MOV);
    unsigned Cycles = AVM::getFixedCycles(AVM::AVMCostKind::CallFar) +
                      Copies * AVM::getFixedCycles(AVM::AVMCostKind::MovUpper);
    return normalizedCycles(Cycles);
  }

public:
  explicit AVMTTIImpl(const AVMTargetMachine *TM, const Function &F)
      : BaseT(TM, F.getDataLayout()), ST(TM->getSubtargetImpl(F)),
        TLI(ST->getTargetLowering()) {}

  unsigned getNumberOfRegisters(unsigned ClassID) const override {
    return ClassID == 1 ? 0 : 12;
  }

  bool shouldBuildLookupTables() const override { return false; }

  bool useFastCCForInternalCall(Function &) const override { return false; }

  InstructionCost getIntImmCost(const APInt &Imm, Type *Ty,
                                TTI::TargetCostKind CostKind) const override {
    if (!Ty->isIntegerTy() || Ty->getIntegerBitWidth() > 64)
      return BaseT::getIntImmCost(Imm, Ty, CostKind);
    unsigned Parts = divideCeil(Ty->getIntegerBitWidth(), 16u);
    APInt Extended = Imm.zextOrTrunc(Parts * 16);
    InstructionCost Cost = 0;
    for (unsigned I = 0; I != Parts; ++I) {
      uint16_t Part = Extended.extractBitsAsZExtValue(16, I * 16);
      bool FitsByte = Part <= 0xff;
      if (CostKind == TTI::TCK_CodeSize)
        Cost += encodedBytes(FitsByte ? AVM::LDI8 : AVM::LDI16);
      else
        Cost += normalizedCycles(
            AVM::getFixedCycles(FitsByte ? AVM::AVMCostKind::Ldi8Upper
                                         : AVM::AVMCostKind::Ldi16Upper));
    }
    return Cost;
  }

  InstructionCost
  getIntrinsicInstrCost(const IntrinsicCostAttributes &ICA,
                        TTI::TargetCostKind CostKind) const override {
    if (ICA.getID() == Intrinsic::bswap &&
        ICA.getReturnType()->isIntegerTy(16)) {
      if (CostKind == TTI::TCK_CodeSize)
        return encodedBytes(AVM::BSWAP16);
      return normalizedCycles(AVM::getFixedCycles(AVM::AVMCostKind::BSwap16));
    }

    auto FloatIntrinsicCost = [&](AVM::AVMCostKind Kind, unsigned Opcode,
                                  bool IsFixed = false) {
      if (CostKind == TTI::TCK_CodeSize)
        return encodedBytes(Opcode);
      unsigned Cycles = IsFixed ? AVM::getFixedCycles(Kind)
                                : AVM::getCycleRange(Kind).Typical;
      return normalizedCycles(Cycles);
    };
    switch (ICA.getID()) {
    case Intrinsic::sqrt:
      return FloatIntrinsicCost(AVM::AVMCostKind::FSqrt, AVM::FSQRT);
    case Intrinsic::fabs:
      return FloatIntrinsicCost(AVM::AVMCostKind::FAbs, AVM::FABS,
                                /*IsFixed=*/true);
    case Intrinsic::minnum:
      return FloatIntrinsicCost(AVM::AVMCostKind::FMin, AVM::FMIN);
    case Intrinsic::maxnum:
      return FloatIntrinsicCost(AVM::AVMCostKind::FMax, AVM::FMAX);
    case Intrinsic::trunc:
      return FloatIntrinsicCost(AVM::AVMCostKind::FTrunc, AVM::FTRUNC);
    case Intrinsic::floor:
      return FloatIntrinsicCost(AVM::AVMCostKind::FFloor, AVM::FFLOOR);
    case Intrinsic::ceil:
      return FloatIntrinsicCost(AVM::AVMCostKind::FCeil, AVM::FCEIL);
    case Intrinsic::round:
      return FloatIntrinsicCost(AVM::AVMCostKind::FRound, AVM::FROUND);
    case Intrinsic::is_fpclass:
      return FloatIntrinsicCost(AVM::AVMCostKind::FClass, AVM::FCLASS);
    default:
      break;
    }

    switch (ICA.getID()) {
    case Intrinsic::sin:
      return FloatIntrinsicCost(AVM::AVMCostKind::SysSinf, AVM::SYS);
    case Intrinsic::cos:
      return FloatIntrinsicCost(AVM::AVMCostKind::SysCosf, AVM::SYS);
    case Intrinsic::atan2:
      return FloatIntrinsicCost(AVM::AVMCostKind::SysAtan2f, AVM::SYS);
    case Intrinsic::tan:
      return FloatIntrinsicCost(AVM::AVMCostKind::SysTanf, AVM::SYS);
    case Intrinsic::exp:
      return FloatIntrinsicCost(AVM::AVMCostKind::SysExpf, AVM::SYS);
    case Intrinsic::log:
      return FloatIntrinsicCost(AVM::AVMCostKind::SysLogf, AVM::SYS);
    case Intrinsic::log2:
      return FloatIntrinsicCost(AVM::AVMCostKind::SysLog2f, AVM::SYS);
    case Intrinsic::log10:
      return FloatIntrinsicCost(AVM::AVMCostKind::SysLog10f, AVM::SYS);
    case Intrinsic::pow:
      return FloatIntrinsicCost(AVM::AVMCostKind::SysPowf, AVM::SYS);
#define AVM_SYS_FIXED_COST(IntrinsicName, Cost)                                \
  case Intrinsic::IntrinsicName:                                               \
    if (CostKind == TTI::TCK_CodeSize)                                         \
      return encodedBytes(AVM::SYS);                                           \
    return normalizedCycles(AVM::getFixedCycles(AVM::AVMCostKind::Cost));
#define AVM_SYS_RANGE_COST(IntrinsicName, Cost)                                \
  case Intrinsic::IntrinsicName:                                               \
    if (CostKind == TTI::TCK_CodeSize)                                         \
      return encodedBytes(AVM::SYS);                                           \
    return normalizedCycles(AVM::getCycleRange(AVM::AVMCostKind::Cost).Typical);
#define AVM_NO_COST(IntrinsicName, Cost)
#define AVM_SYS_INTRINSIC(IntrinsicName, CostKind, Cost)                       \
  CostKind(IntrinsicName, Cost)
#define AVM_NO_INTRINSIC(IntrinsicName, CostKind, Cost)
#define AVM_SYS_DEF(ID, AsmName, PseudoKind, Pseudo, IntrinsicKind,            \
                    IntrinsicName, ServiceCostKind, Cost)                      \
  IntrinsicKind(IntrinsicName, ServiceCostKind, Cost)
#include "AVMSystemCalls.inc"
#undef AVM_SYS_FIXED_COST
#undef AVM_SYS_RANGE_COST
#undef AVM_NO_COST
#undef AVM_SYS_INTRINSIC
#undef AVM_NO_INTRINSIC
    default:
      return BaseT::getIntrinsicInstrCost(ICA, CostKind);
    }
  }

  void getUnrollingPreferences(Loop *L, ScalarEvolution &SE,
                               TTI::UnrollingPreferences &UP,
                               OptimizationRemarkEmitter *ORE) const override;

  InstructionCost getArithmeticInstrCost(
      unsigned Opcode, Type *Ty, TTI::TargetCostKind CostKind,
      TTI::OperandValueInfo Op1Info = {TTI::OK_AnyValue, TTI::OP_None},
      TTI::OperandValueInfo Op2Info = {TTI::OK_AnyValue, TTI::OP_None},
      ArrayRef<const Value *> Args = {},
      const Instruction *CxtI = nullptr) const override {
    if (Ty->isVectorTy())
      return InstructionCost::getInvalid();

    unsigned ISDOpcode = TLI->InstructionOpcodeToISD(Opcode);
    if (CostKind == TTI::TCK_CodeSize) {
      if (Ty->isFloatTy()) {
        switch (ISDOpcode) {
        case ISD::FADD:
          return encodedBytes(AVM::FADD);
        case ISD::FSUB:
          return encodedBytes(AVM::FSUB);
        case ISD::FMUL:
          return encodedBytes(AVM::FMUL);
        case ISD::FDIV:
          return encodedBytes(AVM::FDIV);
        case ISD::FREM:
          return encodedBytes(AVM::SYS);
        case ISD::FNEG:
          return encodedBytes(AVM::FNEG);
        default:
          return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                               Op2Info, Args, CxtI);
        }
      }
      if (Ty->isIntegerTy(16)) {
        switch (ISDOpcode) {
        case ISD::ADD:
          return encodedBytes(AVM::ADD);
        case ISD::SUB:
          return encodedBytes(AVM::SUB);
        case ISD::AND:
          return encodedBytes(AVM::AND);
        case ISD::OR:
          return encodedBytes(AVM::OR);
        case ISD::XOR:
          return encodedBytes(AVM::XOR);
        case ISD::MUL:
          return encodedBytes(AVM::MUL16);
        case ISD::UDIV:
          return encodedBytes(AVM::UDIV16);
        case ISD::UREM:
          return encodedBytes(AVM::UREM16);
        case ISD::SDIV:
          return encodedBytes(AVM::SDIV16);
        case ISD::SREM:
          return encodedBytes(AVM::SREM16);
        case ISD::SHL:
        case ISD::SRL:
        case ISD::SRA: {
          const auto *Count =
              CxtI ? dyn_cast<ConstantInt>(CxtI->getOperand(1)) : nullptr;
          if (Count && Count->isZero())
            return TTI::TCC_Free;
          if (!Count)
            return encodedBytes(ISDOpcode == ISD::SHL   ? AVM::SHL16V
                                : ISDOpcode == ISD::SRL ? AVM::LSR16V
                                                        : AVM::ASR16V);
          return encodedBytes(ISDOpcode == ISD::SHL   ? AVM::LSL16I
                              : ISDOpcode == ISD::SRL ? AVM::LSR16I
                                                      : AVM::ASR16I);
        }
        default:
          return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                               Op2Info, Args, CxtI);
        }
      }
      if (Ty->isIntegerTy(32)) {
        switch (ISDOpcode) {
        case ISD::ADD:
          return encodedBytes(AVM::ADD32);
        case ISD::SUB:
          return encodedBytes(AVM::SUB32);
        case ISD::AND:
          return 2 * encodedBytes(AVM::AND);
        case ISD::OR:
          return 2 * encodedBytes(AVM::OR);
        case ISD::XOR:
          return 2 * encodedBytes(AVM::XOR);
        default:
          return getHelperArithmeticCost(Ty, Opcode, CostKind);
        }
      }
      if (Ty->isIntegerTy(64))
        return getHelperArithmeticCost(Ty, Opcode, CostKind);
      return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                           Op2Info, Args, CxtI);
    }

    using AVM::AVMCostKind;
    auto Normalized = [](unsigned Cycles) {
      return InstructionCost(AVM::normalizeCyclesForTTI(Cycles));
    };
    if (Ty->isFloatTy()) {
      switch (ISDOpcode) {
      case ISD::FADD:
        return Normalized(AVM::getCycleRange(AVMCostKind::FAdd).Typical);
      case ISD::FSUB:
        return Normalized(AVM::getCycleRange(AVMCostKind::FSub).Typical);
      case ISD::FMUL:
        return Normalized(AVM::getCycleRange(AVMCostKind::FMul).Typical);
      case ISD::FDIV:
        return Normalized(AVM::getCycleRange(AVMCostKind::FDiv).Typical);
      case ISD::FREM:
        return Normalized(AVM::getCycleRange(AVMCostKind::SysFmodf).Typical);
      case ISD::FNEG:
        return Normalized(AVM::getFixedCycles(AVMCostKind::FNeg));
      default:
        return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                             Op2Info, Args, CxtI);
      }
    }
    if (Ty->isIntegerTy(32)) {
      switch (ISDOpcode) {
      case ISD::ADD:
        return Normalized(AVM::getFixedCycles(AVMCostKind::Add32));
      case ISD::SUB:
        return Normalized(AVM::getFixedCycles(AVMCostKind::Sub32));
      case ISD::AND:
        return Normalized(2 * AVM::getFixedCycles(AVMCostKind::AndUpper));
      case ISD::OR:
        return Normalized(2 * AVM::getFixedCycles(AVMCostKind::OrUpper));
      case ISD::XOR:
        return Normalized(2 * AVM::getFixedCycles(AVMCostKind::XorUpper));
      default:
        return getHelperArithmeticCost(Ty, Opcode, CostKind);
      }
    }
    if (Ty->isIntegerTy(64))
      return getHelperArithmeticCost(Ty, Opcode, CostKind);
    if (!Ty->isIntegerTy(16))
      return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                           Op2Info, Args, CxtI);
    switch (ISDOpcode) {
    case ISD::ADD:
      return Normalized(AVM::getFixedCycles(AVMCostKind::AddUpper));
    case ISD::SUB:
      return Normalized(AVM::getFixedCycles(AVMCostKind::SubUpper));
    case ISD::AND:
      return Normalized(AVM::getFixedCycles(AVMCostKind::AndUpper));
    case ISD::OR:
      return Normalized(AVM::getFixedCycles(AVMCostKind::OrUpper));
    case ISD::XOR:
      return Normalized(AVM::getFixedCycles(AVMCostKind::XorUpper));
    case ISD::MUL:
      return Normalized(AVM::getFixedCycles(AVMCostKind::Mul16));
    case ISD::UDIV:
      return Normalized(AVM::getCycleRange(AVMCostKind::UDiv16).Typical);
    case ISD::UREM:
      return Normalized(AVM::getCycleRange(AVMCostKind::URem16).Typical);
    case ISD::SDIV:
      return Normalized(AVM::getCycleRange(AVMCostKind::SDiv16).Typical);
    case ISD::SREM:
      return Normalized(AVM::getCycleRange(AVMCostKind::SRem16).Typical);
    case ISD::SHL:
    case ISD::SRL:
    case ISD::SRA:
      break;
    default:
      return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                           Op2Info, Args, CxtI);
    }

    const auto *Count =
        CxtI ? dyn_cast<ConstantInt>(CxtI->getOperand(1)) : nullptr;
    if (!Count)
      return Normalized(60);
    unsigned Amount = Count->getZExtValue();
    if (Amount == 0)
      return TTI::TCC_Free;
    if (Amount > 15)
      return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                           Op2Info, Args, CxtI);

    switch (ISDOpcode) {
    case ISD::SHL:
      if (Amount <= 3)
        return Normalized(Amount * AVM::getFixedCycles(AVMCostKind::AddUpper));
      return Normalized(AVM::getShiftCycles(AVMCostKind::Lsl16I, Amount));
    case ISD::SRL:
      if (Amount == 1)
        return Normalized(AVM::getFixedCycles(AVMCostKind::Lsr16One));
      return Normalized(AVM::getShiftCycles(AVMCostKind::Lsr16I, Amount));
    case ISD::SRA:
      if (Amount == 1)
        return Normalized(AVM::getFixedCycles(AVMCostKind::Asr16One));
      return Normalized(AVM::getShiftCycles(AVMCostKind::Asr16I, Amount));
    default:
      llvm_unreachable("unexpected AVM shift opcode");
    }
  }

  InstructionCost
  getCastInstrCost(unsigned Opcode, Type *Dst, Type *Src,
                   TTI::CastContextHint CCH, TTI::TargetCostKind CostKind,
                   const Instruction *I = nullptr) const override {
    if (Dst->isVectorTy() || Src->isVectorTy())
      return InstructionCost::getInvalid();
    if (CostKind == TTI::TCK_CodeSize) {
      if (Opcode == Instruction::ZExt && Src->isIntegerTy(8) &&
          Dst->isIntegerTy(16))
        return encodedBytes(AVM::ZEXT8);
      if (Opcode == Instruction::SExt && Src->isIntegerTy(8) &&
          Dst->isIntegerTy(16))
        return encodedBytes(AVM::SEXT8);
      if (Opcode == Instruction::ZExt && Src->isIntegerTy(1) &&
          Dst->isIntegerTy(16))
        return encodedBytes(AVM::BOOL);
      if (Opcode == Instruction::Trunc)
        return TTI::TCC_Free;
      if (Opcode == Instruction::SIToFP || Opcode == Instruction::UIToFP) {
        bool Signed = Opcode == Instruction::SIToFP;
        if (Dst->isFloatTy() && Src->isIntegerTy(16))
          return encodedBytes(Signed ? AVM::S16TOF : AVM::U16TOF);
        if (Dst->isFloatTy() && Src->isIntegerTy(32))
          return encodedBytes(Signed ? AVM::S32TOF : AVM::U32TOF);
      }
      if (Opcode == Instruction::FPToSI || Opcode == Instruction::FPToUI) {
        bool Signed = Opcode == Instruction::FPToSI;
        if (Src->isFloatTy() && Dst->isIntegerTy(16))
          return encodedBytes(Signed ? AVM::FTOS16 : AVM::FTOU16);
        if (Src->isFloatTy() && Dst->isIntegerTy(32))
          return encodedBytes(Signed ? AVM::FTOS32 : AVM::FTOU32);
      }
      return BaseT::getCastInstrCost(Opcode, Dst, Src, CCH, CostKind, I);
    }
    using AVM::AVMCostKind;
    auto Normalized = [](AVMCostKind Kind) {
      return InstructionCost(
          AVM::normalizeCyclesForTTI(AVM::getCycleRange(Kind).Typical));
    };
    if (Opcode == Instruction::SIToFP || Opcode == Instruction::UIToFP) {
      bool IsSigned = Opcode == Instruction::SIToFP;
      if (Dst->isFloatTy() && Src->isIntegerTy(16))
        return Normalized(IsSigned ? AVMCostKind::S16ToF : AVMCostKind::U16ToF);
      if (Dst->isFloatTy() && Src->isIntegerTy(32))
        return Normalized(IsSigned ? AVMCostKind::S32ToF : AVMCostKind::U32ToF);
    }
    if (Opcode == Instruction::FPToSI || Opcode == Instruction::FPToUI) {
      bool IsSigned = Opcode == Instruction::FPToSI;
      if (Src->isFloatTy() && Dst->isIntegerTy(16))
        return Normalized(IsSigned ? AVMCostKind::FToS16 : AVMCostKind::FToU16);
      if (Src->isFloatTy() && Dst->isIntegerTy(32))
        return Normalized(IsSigned ? AVMCostKind::FToS32 : AVMCostKind::FToU32);
    }
    if (Opcode == Instruction::ZExt && Src->isIntegerTy(8) &&
        Dst->isIntegerTy(16))
      return normalizedCycles(AVM::getFixedCycles(AVMCostKind::ZExt8));
    if (Opcode == Instruction::SExt && Src->isIntegerTy(8) &&
        Dst->isIntegerTy(16))
      return normalizedCycles(AVM::getFixedCycles(AVMCostKind::SExt8));
    if (Opcode == Instruction::ZExt && Src->isIntegerTy(1) &&
        Dst->isIntegerTy(16))
      return normalizedCycles(AVM::getFixedCycles(AVMCostKind::Bool));
    if (Opcode == Instruction::Trunc)
      return TTI::TCC_Free;
    return BaseT::getCastInstrCost(Opcode, Dst, Src, CCH, CostKind, I);
  }

  InstructionCost getCmpSelInstrCost(
      unsigned Opcode, Type *ValTy, Type *CondTy, CmpInst::Predicate VecPred,
      TTI::TargetCostKind CostKind,
      TTI::OperandValueInfo Op1Info = {TTI::OK_AnyValue, TTI::OP_None},
      TTI::OperandValueInfo Op2Info = {TTI::OK_AnyValue, TTI::OP_None},
      const Instruction *I = nullptr) const override {
    if (ValTy->isVectorTy())
      return InstructionCost::getInvalid();
    if (CostKind == TTI::TCK_CodeSize) {
      if (Opcode == Instruction::ICmp) {
        if (ValTy->isIntegerTy(16) ||
            (ValTy->isPointerTy() && ValTy->getPointerAddressSpace() == 0))
          return encodedBytes(AVM::CMP);
        if (ValTy->isPointerTy() && ValTy->getPointerAddressSpace() == 1)
          return 2 * encodedBytes(AVM::ZEXT8) + encodedBytes(AVM::CMP32);
        if (ValTy->isIntegerTy(32))
          return encodedBytes(AVM::CMP32);
      }
      if (Opcode == Instruction::FCmp && ValTy->isFloatTy())
        return encodedBytes(AVM::FCMP);
      if (Opcode == Instruction::Select) {
        if (ValTy->isIntegerTy(16))
          return encodedBytes(AVM::CMOV_EQ);
        if (ValTy->isIntegerTy(32) || ValTy->isFloatTy() ||
            ValTy->isPointerTy())
          return 2 * encodedBytes(AVM::CMOV_EQ);
      }
      return BaseT::getCmpSelInstrCost(Opcode, ValTy, CondTy, VecPred, CostKind,
                                       Op1Info, Op2Info, I);
    }

    using AVM::AVMCostKind;
    auto Normalized = [](unsigned Cycles) {
      return InstructionCost(AVM::normalizeCyclesForTTI(Cycles));
    };
    if (Opcode == Instruction::ICmp) {
      if (ValTy->isIntegerTy(16))
        return Normalized(AVM::getFixedCycles(AVMCostKind::CmpUpper));
      if (ValTy->isPointerTy() && ValTy->getPointerAddressSpace() == 0)
        return Normalized(AVM::getFixedCycles(AVMCostKind::CmpUpper));
      if (ValTy->isPointerTy() && ValTy->getPointerAddressSpace() == 1)
        return Normalized(2 * AVM::getFixedCycles(AVMCostKind::ZExt8) +
                          AVM::getFixedCycles(AVMCostKind::Cmp32));
      if (ValTy->isIntegerTy(32))
        return Normalized(AVM::getFixedCycles(AVMCostKind::Cmp32));
    }
    if (Opcode == Instruction::FCmp && ValTy->isFloatTy())
      return Normalized(AVM::getCycleRange(AVMCostKind::FCmp).Typical);
    if (Opcode == Instruction::Select) {
      if (ValTy->isIntegerTy(16))
        return Normalized(AVM::getBranchCycles(AVMCostKind::CmovEq, true));
      if (ValTy->isIntegerTy(32) || ValTy->isFloatTy() || ValTy->isPointerTy())
        return Normalized(2 * AVM::getBranchCycles(AVMCostKind::CmovEq, true));
    }
    return BaseT::getCmpSelInstrCost(Opcode, ValTy, CondTy, VecPred, CostKind,
                                     Op1Info, Op2Info, I);
  }

  InstructionCost
  getCFInstrCost(unsigned Opcode, TTI::TargetCostKind CostKind,
                 const Instruction *I = nullptr) const override {
    if (CostKind == TTI::TCK_CodeSize && Opcode == Instruction::Br)
      return encodedBytes(AVM::BREQ8);
    if (CostKind != TTI::TCK_CodeSize && Opcode == Instruction::Br)
      return InstructionCost(AVM::normalizeCyclesForTTI(
          AVM::getBranchCycles(AVM::AVMCostKind::BrEq8, false)));
    return BaseT::getCFInstrCost(Opcode, CostKind, I);
  }

  InstructionCost getMemoryOpCost(
      unsigned Opcode, Type *Src, Align Alignment, unsigned AddressSpace,
      TTI::TargetCostKind CostKind,
      TTI::OperandValueInfo OpInfo = {TTI::OK_AnyValue, TTI::OP_None},
      const Instruction *I = nullptr) const override {
    if (Src->isVectorTy())
      return InstructionCost::getInvalid();
    if (AddressSpace == 1 && Opcode == Instruction::Store)
      return InstructionCost::getInvalid();
    if (CostKind == TTI::TCK_CodeSize) {
      if (AddressSpace == 0 &&
          (Opcode == Instruction::Load || Opcode == Instruction::Store)) {
        if (Src->isIntegerTy(8))
          return encodedBytes(Opcode == Instruction::Load ? AVM::LD8U
                                                          : AVM::ST8);
        if (Src->isIntegerTy(16))
          return encodedBytes(Opcode == Instruction::Load ? AVM::LD16
                                                          : AVM::ST16);
        if (Src->isIntegerTy(32) || Src->isFloatTy())
          return encodedBytes(Opcode == Instruction::Load ? AVM::LD32
                                                          : AVM::ST32);
      }
      if (AddressSpace == 1 && Opcode == Instruction::Load) {
        if (Src->isIntegerTy(8))
          return encodedBytes(AVM::LDP8U);
        if (Src->isIntegerTy(16))
          return encodedBytes(AVM::LDP16);
        if (Src->isPointerTy() && Src->getPointerAddressSpace() == 1)
          return encodedBytes(AVM::LDP24);
        if (Src->isIntegerTy(32) || Src->isFloatTy())
          return encodedBytes(AVM::LDP32);
      }
      return BaseT::getMemoryOpCost(Opcode, Src, Alignment, AddressSpace,
                                    CostKind, OpInfo, I);
    }
    using AVM::AVMCostKind;
    auto Normalized = [](AVMCostKind Kind) {
      return InstructionCost(
          AVM::normalizeCyclesForTTI(AVM::getFixedCycles(Kind)));
    };
    if (AddressSpace == 0 &&
        (Opcode == Instruction::Load || Opcode == Instruction::Store)) {
      if (Src->isIntegerTy(8) || Src->isIntegerTy(16))
        return Normalized(Opcode == Instruction::Load
                              ? (Src->isIntegerTy(8) ? AVMCostKind::Ld8UUpper
                                                     : AVMCostKind::Ld16Upper)
                              : (Src->isIntegerTy(8) ? AVMCostKind::St8Upper
                                                     : AVMCostKind::St16Upper));
      if (Src->isIntegerTy(32) || Src->isFloatTy())
        return Normalized(Opcode == Instruction::Load ? AVMCostKind::Ld32
                                                      : AVMCostKind::St32);
    }
    if (AddressSpace != 1 || Opcode != Instruction::Load)
      return BaseT::getMemoryOpCost(Opcode, Src, Alignment, AddressSpace,
                                    CostKind, OpInfo, I);
    AVMCostKind Kind;
    if (Src->isIntegerTy(8))
      Kind = AVMCostKind::Ldp8U;
    else if (Src->isIntegerTy(16))
      Kind = AVMCostKind::Ldp16;
    else if (Src->isPointerTy() && Src->getPointerAddressSpace() == 1)
      Kind = AVMCostKind::Ldp24;
    else if (Src->isIntegerTy(32) || Src->isFloatTy())
      Kind = AVMCostKind::Ldp32;
    else
      return BaseT::getMemoryOpCost(Opcode, Src, Alignment, AddressSpace,
                                    CostKind, OpInfo, I);
    return Normalized(Kind);
  }
};
} // namespace llvm

#endif
