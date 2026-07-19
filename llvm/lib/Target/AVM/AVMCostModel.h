//===-- AVMCostModel.h - AVM interpreter cost model ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVM_AVMCOSTMODEL_H
#define LLVM_LIB_TARGET_AVM_AVMCOSTMODEL_H

#include "llvm/Support/BranchProbability.h"

#include <cstdint>

namespace llvm::AVM {

enum class AVMCostKind : uint16_t {
#define AVM_FIXED_COST(Name, Cycles) Name,
#define AVM_RANGE_COST(Name, Typical, Minimum, Maximum) Name,
#define AVM_BRANCH_COST(Name, NotTaken, Taken) Name,
#define AVM_SHIFT_COST(Name, Count, Cycles)
#include "AVMCycleCosts.def"
#undef AVM_FIXED_COST
#undef AVM_RANGE_COST
#undef AVM_BRANCH_COST
#undef AVM_SHIFT_COST
  Shl16V,
  Lsr16V,
  Asr16V,
  Lsl16I,
  Lsr16I,
  Asr16I,
};

struct AVMCycleRange {
  unsigned Typical;
  unsigned Minimum;
  unsigned Maximum;
};

unsigned getFixedCycles(AVMCostKind Kind);
AVMCycleRange getCycleRange(AVMCostKind Kind);
unsigned getShiftCycles(AVMCostKind Kind, unsigned Count);
unsigned getBranchCycles(AVMCostKind Kind, bool Taken);
unsigned getExpectedBranchCycles(AVMCostKind Kind,
                                 BranchProbability TakenProbability);
unsigned normalizeCyclesForTTI(unsigned Cycles);

} // namespace llvm::AVM

#endif // LLVM_LIB_TARGET_AVM_AVMCOSTMODEL_H
