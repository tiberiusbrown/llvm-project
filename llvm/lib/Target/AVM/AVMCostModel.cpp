//===-- AVMCostModel.cpp - AVM interpreter cost model ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVMCostModel.h"
#include "llvm/Support/ErrorHandling.h"

#include <array>
#include <cstdint>

using namespace llvm;
using namespace llvm::AVM;

namespace {

constexpr unsigned InvalidCycles = 0;

#define AVM_SHIFT_TABLE(Table, Kind)                                           \
  const std::array<unsigned, 16> &Table() {                                    \
    static const std::array<unsigned, 16> Values = [] {                        \
      std::array<unsigned, 16> Costs{};                                        \
      (void)Costs;                                                             \
      return Costs;                                                            \
    }();                                                                       \
    return Values;                                                             \
  }

// Keep the six arrays spelled independently: each is populated by the same
// measured input file, with nonmatching shift kinds discarded at compile time.
const std::array<unsigned, 16> &shl16VTable() {
  static const std::array<unsigned, 16> Values = [] {
    std::array<unsigned, 16> Costs{};
#define AVM_SHIFT_COST(Name, Count, Cycles)                                    \
  if constexpr (AVMCostKind::Name == AVMCostKind::Shl16V)                      \
    Costs[Count] = Cycles;
#include "AVMCycleCosts.def"
#undef AVM_SHIFT_COST
    return Costs;
  }();
  return Values;
}
const std::array<unsigned, 16> &lsr16VTable() {
  static const std::array<unsigned, 16> Values = [] {
    std::array<unsigned, 16> Costs{};
#define AVM_SHIFT_COST(Name, Count, Cycles)                                    \
  if constexpr (AVMCostKind::Name == AVMCostKind::Lsr16V)                      \
    Costs[Count] = Cycles;
#include "AVMCycleCosts.def"
#undef AVM_SHIFT_COST
    return Costs;
  }();
  return Values;
}
const std::array<unsigned, 16> &asr16VTable() {
  static const std::array<unsigned, 16> Values = [] {
    std::array<unsigned, 16> Costs{};
#define AVM_SHIFT_COST(Name, Count, Cycles)                                    \
  if constexpr (AVMCostKind::Name == AVMCostKind::Asr16V)                      \
    Costs[Count] = Cycles;
#include "AVMCycleCosts.def"
#undef AVM_SHIFT_COST
    return Costs;
  }();
  return Values;
}
const std::array<unsigned, 16> &lsl16ITable() {
  static const std::array<unsigned, 16> Values = [] {
    std::array<unsigned, 16> Costs{};
#define AVM_SHIFT_COST(Name, Count, Cycles)                                    \
  if constexpr (AVMCostKind::Name == AVMCostKind::Lsl16I)                      \
    Costs[Count] = Cycles;
#include "AVMCycleCosts.def"
#undef AVM_SHIFT_COST
    return Costs;
  }();
  return Values;
}
const std::array<unsigned, 16> &lsr16ITable() {
  static const std::array<unsigned, 16> Values = [] {
    std::array<unsigned, 16> Costs{};
#define AVM_SHIFT_COST(Name, Count, Cycles)                                    \
  if constexpr (AVMCostKind::Name == AVMCostKind::Lsr16I)                      \
    Costs[Count] = Cycles;
#include "AVMCycleCosts.def"
#undef AVM_SHIFT_COST
    return Costs;
  }();
  return Values;
}
const std::array<unsigned, 16> &asr16ITable() {
  static const std::array<unsigned, 16> Values = [] {
    std::array<unsigned, 16> Costs{};
#define AVM_SHIFT_COST(Name, Count, Cycles)                                    \
  if constexpr (AVMCostKind::Name == AVMCostKind::Asr16I)                      \
    Costs[Count] = Cycles;
#include "AVMCycleCosts.def"
#undef AVM_SHIFT_COST
    return Costs;
  }();
  return Values;
}

const std::array<unsigned, 16> *getShiftTable(AVMCostKind Kind) {
  switch (Kind) {
  case AVMCostKind::Shl16V:
    return &shl16VTable();
  case AVMCostKind::Lsr16V:
    return &lsr16VTable();
  case AVMCostKind::Asr16V:
    return &asr16VTable();
  case AVMCostKind::Lsl16I:
    return &lsl16ITable();
  case AVMCostKind::Lsr16I:
    return &lsr16ITable();
  case AVMCostKind::Asr16I:
    return &asr16ITable();
  default:
    return nullptr;
  }
}

} // namespace

unsigned llvm::AVM::getFixedCycles(AVMCostKind Kind) {
  switch (Kind) {
#define AVM_FIXED_COST(Name, Cycles)                                           \
  case AVMCostKind::Name:                                                      \
    return Cycles;
#include "AVMCycleCosts.def"
#undef AVM_FIXED_COST
  default:
    return InvalidCycles;
  }
}

AVMCycleRange llvm::AVM::getCycleRange(AVMCostKind Kind) {
  switch (Kind) {
#define AVM_RANGE_COST(Name, Typical, Minimum, Maximum)                        \
  case AVMCostKind::Name:                                                      \
    return {Typical, Minimum, Maximum};
#include "AVMCycleCosts.def"
#undef AVM_RANGE_COST
  default:
    return {InvalidCycles, InvalidCycles, InvalidCycles};
  }
}

unsigned llvm::AVM::getShiftCycles(AVMCostKind Kind, unsigned Count) {
  if (Count > 15)
    llvm_unreachable("AVM shift count must be in the range 0 through 15");
  const std::array<unsigned, 16> *Table = getShiftTable(Kind);
  return Table ? (*Table)[Count] : InvalidCycles;
}

unsigned llvm::AVM::getBranchCycles(AVMCostKind Kind, bool Taken) {
  switch (Kind) {
#define AVM_BRANCH_COST(Name, NotTaken, TakenCycles)                           \
  case AVMCostKind::Name:                                                      \
    return Taken ? TakenCycles : NotTaken;
#include "AVMCycleCosts.def"
#undef AVM_BRANCH_COST
  default:
    return InvalidCycles;
  }
}

unsigned
llvm::AVM::getExpectedBranchCycles(AVMCostKind Kind,
                                   BranchProbability TakenProbability) {
  const uint64_t NotTaken = getBranchCycles(Kind, false);
  const uint64_t Taken = getBranchCycles(Kind, true);
  const uint64_t N = TakenProbability.getNumerator();
  const uint64_t D = TakenProbability.getDenominator();
  return static_cast<unsigned>((NotTaken * (D - N) + Taken * N + D / 2) / D);
}

unsigned llvm::AVM::normalizeCyclesForTTI(unsigned Cycles) {
  return std::max(1U, (Cycles + 8) / 17);
}
