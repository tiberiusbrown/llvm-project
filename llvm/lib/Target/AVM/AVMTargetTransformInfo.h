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
    if (!Ty->isIntegerTy(16) || CostKind == TTI::TCK_CodeSize)
      return BaseT::getArithmeticInstrCost(Opcode, Ty, CostKind, Op1Info,
                                           Op2Info, Args, CxtI);

    using AVM::AVMCostKind;
    auto Normalized = [](unsigned Cycles) {
      return InstructionCost(AVM::normalizeCyclesForTTI(Cycles));
    };
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
};
} // namespace llvm

#endif
