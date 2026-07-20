//===-- AVMTargetTransformInfo.h - AVM TTI implementation ------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMTARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_AVM_AVMTARGETTRANSFORMINFO_H

#include "AVMCostModel.h"
#include "AVMSubtarget.h"
#include "AVMTargetMachine.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"

namespace llvm {
class AVMTTIImpl final : public BasicTTIImplBase<AVMTTIImpl> {
  using BaseT = BasicTTIImplBase<AVMTTIImpl>;
  using TTI = TargetTransformInfo;
  friend BaseT;

  const AVMSubtarget *ST;
  const AVMTargetLowering *TLI;

  const AVMSubtarget *getST() const { return ST; }
  const AVMTargetLowering *getTLI() const { return TLI; }

public:
  explicit AVMTTIImpl(const AVMTargetMachine *TM, const Function &F)
      : BaseT(TM, F.getDataLayout()), ST(TM->getSubtargetImpl(F)),
        TLI(ST->getTargetLowering()) {}

  InstructionCost getArithmeticInstrCost(
      unsigned Opcode, Type *Ty, TTI::TargetCostKind CostKind,
      TTI::OperandValueInfo Op1Info = {TTI::OK_AnyValue, TTI::OP_None},
      TTI::OperandValueInfo Op2Info = {TTI::OK_AnyValue, TTI::OP_None},
      ArrayRef<const Value *> Args = {},
      const Instruction *CxtI = nullptr) const override {
    if (CostKind == TTI::TCK_CodeSize)
      return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                           Op2Info, Args, CxtI);

    using AVM::AVMCostKind;
    auto Normalized = [](unsigned Cycles) {
      return InstructionCost(AVM::normalizeCyclesForTTI(Cycles));
    };
    if (Ty->isFloatTy()) {
      switch (TLI->InstructionOpcodeToISD(Opcode)) {
      case ISD::FADD:
        return Normalized(AVM::getCycleRange(AVMCostKind::FAdd).Typical);
      case ISD::FSUB:
        return Normalized(AVM::getCycleRange(AVMCostKind::FSub).Typical);
      case ISD::FMUL:
        return Normalized(AVM::getCycleRange(AVMCostKind::FMul).Typical);
      case ISD::FDIV:
        return Normalized(AVM::getCycleRange(AVMCostKind::FDiv).Typical);
      default:
        return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                             Op2Info, Args, CxtI);
      }
    }
    if (Ty->isIntegerTy(32)) {
      switch (TLI->InstructionOpcodeToISD(Opcode)) {
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
        return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                             Op2Info, Args, CxtI);
      }
    }
    if (!Ty->isIntegerTy(16))
      return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                           Op2Info, Args, CxtI);
    switch (TLI->InstructionOpcodeToISD(Opcode)) {
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

    switch (TLI->InstructionOpcodeToISD(Opcode)) {
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
    if (CostKind == TTI::TCK_CodeSize)
      return BaseT::getCastInstrCost(Opcode, Dst, Src, CCH, CostKind, I);
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
    return BaseT::getCastInstrCost(Opcode, Dst, Src, CCH, CostKind, I);
  }

  InstructionCost getMemoryOpCost(
      unsigned Opcode, Type *Src, Align Alignment, unsigned AddressSpace,
      TTI::TargetCostKind CostKind,
      TTI::OperandValueInfo OpInfo = {TTI::OK_AnyValue, TTI::OP_None},
      const Instruction *I = nullptr) const override {
    if (AddressSpace != 1 || Opcode != Instruction::Load ||
        CostKind == TTI::TCK_CodeSize)
      return BaseT::getMemoryOpCost(Opcode, Src, Alignment, AddressSpace,
                                    CostKind, OpInfo, I);
    using AVM::AVMCostKind;
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
    return InstructionCost(
        AVM::normalizeCyclesForTTI(AVM::getFixedCycles(Kind)));
  }
};
} // namespace llvm

#endif
