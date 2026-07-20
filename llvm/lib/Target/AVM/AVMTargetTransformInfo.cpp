//===-- AVMTargetTransformInfo.cpp - AVM TTI implementation -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVMTargetTransformInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IntrinsicsAVM.h"

using namespace llvm;

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
