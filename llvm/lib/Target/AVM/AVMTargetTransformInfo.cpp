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
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IntrinsicsAVM.h"

using namespace llvm;

TTI::AddressingModeKind
AVMTTIImpl::getPreferredAddressingMode(const Loop *L,
                                       ScalarEvolution *SE) const {
  if (!L || !SE || !L->isInnermost())
    return TTI::AMK_None;

  unsigned FoldableAccesses = 0;

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

      const auto *AddRec = dyn_cast<SCEVAddRecExpr>(SE->getSCEV(Pointer));

      // Loop-invariant accesses and recurrences belonging to another loop do
      // not participate in this loop's addressing-mode decision.
      if (!AddRec || AddRec->getLoop() != L)
        continue;

      // AVM data-space post-index folding currently exists only for i8 and
      // i16. Because the preference affects the entire loop, reject the loop
      // if any loop-carried memory stream uses another address space or type.
      if (AddressSpace != 0 ||
          (!AccessTy->isIntegerTy(8) && !AccessTy->isIntegerTy(16)))
        return TTI::AMK_None;

      const auto *Step = dyn_cast<SCEVConstant>(AddRec->getStepRecurrence(*SE));
      int64_t Width = AccessTy->getIntegerBitWidth() / 8;

      // AVM post-index instructions increment by exactly the access width.
      // Runtime strides, negative strides, and other constant strides cannot
      // fold and must not receive a post-index preference.
      if (!Step || Step->getAPInt().getSExtValue() != Width)
        return TTI::AMK_None;

      // AVM has only eight 16-bit registers. More than three simultaneous
      // memory streams consistently creates enough pointer pressure to
      // outweigh the addressing savings.
      if (++FoldableAccesses > 3)
        return TTI::AMK_None;
    }
  }

  return FoldableAccesses != 0 ? TTI::AMK_PostIndexed : TTI::AMK_None;
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
