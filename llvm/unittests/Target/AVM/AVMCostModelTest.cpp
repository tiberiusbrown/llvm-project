//===- AVMCostModelTest.cpp - AVM interpreter cost model tests -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVMCostModel.h"
#include "AVM.h"

#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "gtest/gtest.h"

#include <array>

using namespace llvm;
using namespace llvm::AVM;

TEST(AVMCostModelTest, FixedAndRangeCosts) {
  EXPECT_EQ(getFixedCycles(AVMCostKind::MovUpper), 17U);
  EXPECT_EQ(getFixedCycles(AVMCostKind::Ldp32PostInc), 357U);
  EXPECT_EQ(getFixedCycles(AVMCostKind::Ld8UDisplaced), 54U);
  EXPECT_EQ(getFixedCycles(AVMCostKind::Ld16Displaced), 55U);
  EXPECT_EQ(getFixedCycles(AVMCostKind::St8Displaced), 54U);
  EXPECT_EQ(getFixedCycles(AVMCostKind::St16Displaced), 55U);

  const AVMCycleRange Divide = getCycleRange(AVMCostKind::UDiv16);
  EXPECT_EQ(Divide.Typical, 219U);
  EXPECT_EQ(Divide.Minimum, 59U);
  EXPECT_EQ(Divide.Maximum, 251U);
}

TEST(AVMCostModelTest, SystemServiceMetadataReferencesMeasuredCosts) {
#define AVM_SYS_FIXED_COST(Intrinsic, Cost)                                    \
  EXPECT_NE(getFixedCycles(AVMCostKind::Cost), 0U);
#define AVM_SYS_RANGE_COST(Intrinsic, Cost)                                    \
  EXPECT_NE(getCycleRange(AVMCostKind::Cost).Typical, 0U);
#define AVM_NO_COST(Intrinsic, Cost)
#define AVM_SYS_INTRINSIC(Intrinsic, CostKind, Cost) CostKind(Intrinsic, Cost)
#define AVM_NO_INTRINSIC(Intrinsic, CostKind, Cost)
#define AVM_SYS_DEF(ID, AsmName, PseudoKind, Pseudo, IntrinsicKind, Intrinsic, \
                    CostKind, Cost)                                            \
  IntrinsicKind(Intrinsic, CostKind, Cost)
#include "AVMSystemCalls.inc"
#undef AVM_SYS_FIXED_COST
#undef AVM_SYS_RANGE_COST
#undef AVM_NO_COST
#undef AVM_SYS_INTRINSIC
#undef AVM_NO_INTRINSIC
}

TEST(AVMCostModelTest, ShiftCosts) {
  constexpr std::array<unsigned, 16> Shl16V = {42, 45, 50, 55, 60, 65, 70, 75,
                                               44, 47, 52, 57, 62, 67, 72, 77};
  constexpr std::array<unsigned, 16> Lsr16V = {42, 45, 50, 55, 60, 65, 70, 75,
                                               44, 47, 52, 57, 62, 67, 72, 77};
  constexpr std::array<unsigned, 16> Asr16V = {42, 45, 50, 55, 60, 65, 70, 75,
                                               45, 48, 53, 58, 63, 68, 73, 78};
  constexpr std::array<unsigned, 16> Lsl16I = {44, 47, 52, 57, 62, 67, 72, 77,
                                               46, 49, 54, 59, 64, 69, 74, 79};
  constexpr std::array<unsigned, 16> Lsr16I = {44, 47, 52, 57, 62, 67, 72, 77,
                                               46, 49, 54, 59, 64, 69, 74, 79};
  constexpr std::array<unsigned, 16> Asr16I = {44, 47, 52, 57, 62, 67, 72, 77,
                                               47, 50, 55, 60, 65, 70, 75, 80};

  const std::array Kinds = {AVMCostKind::Shl16V, AVMCostKind::Lsr16V,
                            AVMCostKind::Asr16V, AVMCostKind::Lsl16I,
                            AVMCostKind::Lsr16I, AVMCostKind::Asr16I};
  const std::array<const std::array<unsigned, 16> *, 6> Expected = {
      &Shl16V, &Lsr16V, &Asr16V, &Lsl16I, &Lsr16I, &Asr16I};

  for (unsigned Kind = 0; Kind != Kinds.size(); ++Kind)
    for (unsigned Count = 0; Count != 16; ++Count)
      EXPECT_EQ(getShiftCycles(Kinds[Kind], Count), (*Expected[Kind])[Count]);
}

TEST(AVMCostModelTest, BranchCostsAndTTINormalization) {
  constexpr AVMCostKind Kind = AVMCostKind::BrEq8;
  EXPECT_EQ(getBranchCycles(Kind, false), 35U);
  EXPECT_EQ(getBranchCycles(Kind, true), 128U);
  EXPECT_EQ(getExpectedBranchCycles(Kind, BranchProbability(0, 1)), 35U);
  EXPECT_EQ(getExpectedBranchCycles(Kind, BranchProbability(1, 2)), 82U);
  EXPECT_EQ(getExpectedBranchCycles(Kind, BranchProbability(1, 1)), 128U);
  EXPECT_EQ(normalizeCyclesForTTI(0), 1U);
  EXPECT_EQ(normalizeCyclesForTTI(17), 1U);
  EXPECT_EQ(normalizeCyclesForTTI(18), 1U);
  EXPECT_EQ(normalizeCyclesForTTI(26), 2U);
}

TEST(AVMCostModelTest, SchedulingModelUsesEncodedAddForm) {
  LLVMInitializeAVMTargetInfo();
  LLVMInitializeAVMTarget();
  LLVMInitializeAVMTargetMC();

  Triple TT("avm-unknown-arduboyfx");
  std::string Error;
  const Target *T = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(T, nullptr) << Error;

  std::unique_ptr<TargetMachine> TM(
      T->createTargetMachine(TT, "avm1", "", TargetOptions(), std::nullopt,
                             std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(TM, nullptr);

  const MCSubtargetInfo *STI = TM->getMCSubtargetInfo();
  const MCInstrInfo *MII = TM->getMCInstrInfo();
  ASSERT_NE(STI, nullptr);
  ASSERT_NE(MII, nullptr);

  auto GetLatency = [&](unsigned Opcode) {
    const MCSchedModel &Model = STI->getSchedModel();
    const MCSchedClassDesc *SC =
        Model.getSchedClassDesc(MII->get(Opcode).getSchedClass());
    unsigned Latency = 0;
    for (unsigned I = 0; I != SC->NumWriteLatencyEntries; ++I)
      Latency = std::max(
          Latency,
          static_cast<unsigned>(STI->getWriteLatencyEntry(SC, I)->Cycles));
    return Latency;
  };

  EXPECT_EQ(GetLatency(AVM::ADD), 17U);
  EXPECT_EQ(GetLatency(AVM::ADD_RR), 38U);
}
