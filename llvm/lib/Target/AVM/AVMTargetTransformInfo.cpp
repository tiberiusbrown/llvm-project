//===-- AVMTargetTransformInfo.cpp - AVM TTI implementation -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVMTargetTransformInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IntrinsicsAVM.h"

#include <optional>

using namespace llvm;

TTI::AddressingModeKind
AVMTTIImpl::getPreferredAddressingMode(const Loop *L,
                                       ScalarEvolution *SE) const {
  if (!L || !SE || !L->isInnermost())
    return TTI::AMK_None;

  unsigned LoopCarriedAccesses = 0;
  unsigned FoldableAccesses = 0;

  const Value *FirstObject = nullptr;
  bool AllSameObject = true;

  std::optional<int64_t> CommonStep;
  bool AllSameStep = true;
  std::optional<bool> PositiveDirection;

  for (BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      Value *Pointer = nullptr;
      Type *AccessTy = nullptr;
      unsigned AddressSpace = 0;

      if (auto *Load = dyn_cast<LoadInst>(&I)) {
        Pointer = Load->getPointerOperand();
        AccessTy = Load->getType();
        AddressSpace = Load->getPointerAddressSpace();
      } else if (auto *Store = dyn_cast<StoreInst>(&I)) {
        Pointer = Store->getPointerOperand();
        AccessTy = Store->getValueOperand()->getType();
        AddressSpace = Store->getPointerAddressSpace();
      } else {
        continue;
      }

      const SCEV *PointerSCEV = SE->getSCEV(Pointer);

      // Loop-invariant accesses consume no loop-carried pointer register and
      // do not participate in this decision.
      if (SE->isLoopInvariant(PointerSCEV, L))
        continue;

      // A non-affine loop-varying address means that scalar-index expressions
      // still have to remain live. Requesting additional pointer recurrences
      // in that situation increases AVM register pressure.
      const auto *AddRec = dyn_cast<SCEVAddRecExpr>(PointerSCEV);
      if (!AddRec || AddRec->getLoop() != L)
        return TTI::AMK_None;

      // This policy currently applies only to ordinary data-space integer
      // accesses. Floating-point loops were a measured negative case.
      if (AddressSpace != 0 ||
          (!AccessTy->isIntegerTy(8) && !AccessTy->isIntegerTy(16) &&
           !AccessTy->isIntegerTy(32)))
        return TTI::AMK_None;

      const auto *Step = dyn_cast<SCEVConstant>(AddRec->getStepRecurrence(*SE));
      if (!Step)
        return TTI::AMK_None;

      int64_t StepValue = Step->getAPInt().getSExtValue();

      if (StepValue == 0)
        return TTI::AMK_None;

      bool IsPositive = StepValue > 0;
      if (!PositiveDirection)
        PositiveDirection = IsPositive;
      else if (*PositiveDirection != IsPositive)
        return TTI::AMK_None;

      ++LoopCarriedAccesses;

      if (!IsPositive &&
          ((!AccessTy->isIntegerTy(8) && !AccessTy->isIntegerTy(16)) ||
           StepValue !=
               -static_cast<int64_t>(AccessTy->getIntegerBitWidth() / 8)))
        return TTI::AMK_None;

      const Value *Object = getUnderlyingObject(Pointer);
      if (!FirstObject)
        FirstObject = Object;
      else if (Object != FirstObject)
        AllSameObject = false;

      if (!CommonStep)
        CommonStep = StepValue;
      else if (*CommonStep != StepValue)
        AllSameStep = false;

      if ((AccessTy->isIntegerTy(8) || AccessTy->isIntegerTy(16)) &&
          StepValue == static_cast<int64_t>(AccessTy->getIntegerBitWidth() / 8))
        ++FoldableAccesses;
    }
  }

  if (LoopCarriedAccesses == 0)
    return TTI::AMK_None;

  if (!*PositiveDirection)
    return LoopCarriedAccesses <= 3 ? TTI::AMK_PreIndexed : TTI::AMK_None;

  // A small loop may profit when at least one stream becomes a native AVM
  // post-increment access. Non-foldable i32 streams are allowed in this case,
  // but the total number of loop-carried accesses remains capped at three.
  bool SmallMixedStreamSet = FoldableAccesses != 0 && LoopCarriedAccesses <= 3;

  // Several constant-offset fields of one object should share one base pointer.
  // Four covers the current particle and stack-stencil cases without allowing
  // arbitrary high-pressure aggregate loops.
  bool SingleObjectFixedStride =
      AllSameObject && AllSameStep && LoopCarriedAccesses <= 4;

  return SmallMixedStreamSet || SingleObjectFixedStride ? TTI::AMK_PostIndexed
                                                        : TTI::AMK_None;
}

void AVMTTIImpl::getUnrollingPreferences(Loop *L, ScalarEvolution &SE,
                                         TTI::UnrollingPreferences &UP,
                                         OptimizationRemarkEmitter *ORE) const {
  auto IsExpensiveService = [](const CallBase &Call) {
    const auto *II = dyn_cast<IntrinsicInst>(&Call);
    if (!II)
      return false;
    switch (II->getIntrinsicID()) {
    case Intrinsic::sqrt:
    case Intrinsic::sin:
    case Intrinsic::cos:
    case Intrinsic::atan2:
    case Intrinsic::tan:
    case Intrinsic::exp:
    case Intrinsic::log:
    case Intrinsic::log2:
    case Intrinsic::log10:
    case Intrinsic::pow:
    case Intrinsic::avm_sinf:
    case Intrinsic::avm_cosf:
    case Intrinsic::avm_atan2f:
    case Intrinsic::avm_tanf:
    case Intrinsic::avm_expf:
    case Intrinsic::avm_logf:
    case Intrinsic::avm_log2f:
    case Intrinsic::avm_log10f:
    case Intrinsic::avm_powf:
    case Intrinsic::avm_hypotf:
    case Intrinsic::avm_fmodf:
      return true;
    default:
      return false;
    }
  };

  UP.MaxIterationsCountToAnalyze =
    std::max(UP.MaxIterationsCountToAnalyze, 64u);

  for (BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      bool BlocksUnrolling = false;
      if (const auto *Load = dyn_cast<LoadInst>(&I))
        BlocksUnrolling = Load->getPointerAddressSpace() == 1;
      else if (const auto *Call = dyn_cast<CallBase>(&I))
        BlocksUnrolling = IsExpensiveService(*Call);
      else
        BlocksUnrolling = I.getOpcode() == Instruction::UDiv ||
                          I.getOpcode() == Instruction::SDiv ||
                          I.getOpcode() == Instruction::URem ||
                          I.getOpcode() == Instruction::SRem ||
                          I.getOpcode() == Instruction::FDiv;
      if (!BlocksUnrolling)
        continue;

      // Generic program-memory unrolling remains disabled. The dedicated
      // AVMProgramMemoryWidening pass performs only validated factor-two
      // unrolling that is immediately followed by load combining.
      UP.Threshold = 0;
      UP.PartialThreshold = 0;
      UP.OptSizeThreshold = 0;
      UP.PartialOptSizeThreshold = 0;
      UP.MaxCount = 1;
      UP.FullUnrollMaxCount = 1;
      UP.Partial = false;
      UP.Runtime = false;
      return;
    }
  }
}
