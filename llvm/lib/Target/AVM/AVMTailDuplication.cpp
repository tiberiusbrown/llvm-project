//===-- AVMTailDuplication.cpp - Late AVM tail duplication ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Duplicate small, side-effect-free post-RA tails into predecessors that
// otherwise pay for an unconditional transfer.  AVM's interpreter makes even
// a short unconditional jump expensive enough that this is profitable within
// deliberately small code-growth limits.
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MBFIWrapper.h"
#include "llvm/CodeGen/MachineBlockFrequencyInfo.h"
#include "llvm/CodeGen/MachineBranchProbabilityInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/TailDuplicator.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"

#include <cstdint>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "avm-tail-duplication"
#define PASS_NAME "AVM late tail duplication"

static bool EnableAVMTailDuplication = true;

static cl::opt<unsigned> AVMTailDupMaxInsts(
    "avm-tail-dup-max-insts", cl::Hidden,
    cl::desc("Override the AVM tail-duplication non-branch instruction limit"),
    cl::init(0));

static cl::opt<unsigned> AVMTailDupMaxBytes(
    "avm-tail-dup-max-bytes", cl::Hidden,
    cl::desc("Override the AVM tail-duplication encoded-byte limit"),
    cl::init(0));

namespace {

enum class JumpKind { Short, Long, Far };

struct BlockOffsets {
  DenseMap<const MachineBasicBlock *, uint64_t> Starts;
  uint64_t InsertionEnd = 0;
  uint64_t Growth = 0;

  uint64_t adjusted(uint64_t Offset) const {
    return Offset >= InsertionEnd ? Offset + Growth : Offset;
  }

  uint64_t adjusted(const MachineBasicBlock &MBB) const {
    return adjusted(Starts.lookup(&MBB));
  }
};

struct TailMetrics {
  unsigned NonBranchInstructions = 0;
  unsigned NonBranchBytes = 0;
  unsigned DuplicatedBytes = 0;
  bool HasExplicitUnconditionalBranch = false;
};

struct Profitability {
  unsigned OriginalCycles = 0;
  unsigned DuplicatedCycles = 0;
  unsigned RangePenalty = 0;
  JumpKind EliminatedJump = JumpKind::Short;

  int64_t savings() const {
    return static_cast<int64_t>(OriginalCycles) -
           static_cast<int64_t>(DuplicatedCycles);
  }
};

struct Candidate {
  MachineBasicBlock *Predecessor;
  MachineBasicBlock *Tail;
};

class AVMTailDuplication final : public MachineFunctionPass {
  CodeGenOptLevel OptLevel;

public:
  static char ID;

  explicit AVMTailDuplication(
      CodeGenOptLevel OptLevel = CodeGenOptLevel::Default)
      : MachineFunctionPass(ID), OptLevel(OptLevel) {}

  StringRef getPassName() const override { return PASS_NAME; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineBranchProbabilityInfoWrapperPass>();
    AU.addRequired<MachineBlockFrequencyInfoWrapperPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  static bool isExplicitUnconditionalBranch(const MachineInstr &MI);
  static bool isConditionalBranch(const MachineInstr &MI);
  static bool isBranchConditionSetup(const MachineInstr &MI);
  static bool isAllowedUnmodeledPseudo(const MachineInstr &MI);
  static const char *jumpKindName(JumpKind Kind);
  static unsigned weightedCycles(unsigned Cycles, BranchProbability P);

  static uint64_t instructionOffset(const MachineBasicBlock &MBB,
                                    const MachineInstr &Target,
                                    const AVMInstrInfo &TII,
                                    uint64_t BlockStart);
  static BlockOffsets computeBlockOffsets(const MachineFunction &MF,
                                          const AVMInstrInfo &TII);
  static JumpKind jumpKind(uint64_t Source, uint64_t Target,
                           const MachineInstr *MI = nullptr);
  static unsigned jumpCycles(JumpKind Kind);
  static unsigned conditionalCycles(uint64_t Source, uint64_t Target,
                                    BranchProbability TakenProbability);

  static std::optional<TailMetrics>
  checkTailLegality(const MachineBasicBlock &Tail, const AVMInstrInfo &TII,
                    unsigned MaxInstructions, unsigned MaxBytes,
                    StringRef &Reason);

  static unsigned transferCost(const MachineBasicBlock &Tail,
                               const MachineBasicBlock &Placement,
                               uint64_t BranchOffset,
                               const BlockOffsets &Offsets,
                               const MachineBranchProbabilityInfo &MBPI,
                               const AVMInstrInfo &TII, bool IsDuplicated);

  static bool branchRangeDegrades(const MachineFunction &MF,
                                  const MachineBasicBlock &Predecessor,
                                  const MachineBasicBlock &Tail,
                                  const BlockOffsets &OldOffsets,
                                  const BlockOffsets &NewOffsets,
                                  const AVMInstrInfo &TII);

  static bool
  tryPlaceTailAsFallthrough(MachineFunction &MF, MachineBasicBlock &Predecessor,
                            MachineBasicBlock &Tail, MachineInstr &Jump,
                            const MachineBranchProbabilityInfo &MBPI,
                            const MachineBlockFrequencyInfo &MBFI,
                            const AVMInstrInfo &TII, double &OriginalScore,
                            double &PlacedScore, StringRef &Reason);

  static unsigned branchRangePenalty(const MachineFunction &MF,
                                     const MachineBasicBlock &Predecessor,
                                     const MachineBasicBlock &Tail,
                                     const BlockOffsets &Offsets,
                                     const MachineBranchProbabilityInfo &MBPI,
                                     const AVMInstrInfo &TII);

  static Profitability estimateProfitability(
      const MachineFunction &MF, const MachineBasicBlock &Predecessor,
      const MachineBasicBlock &Tail, const MachineInstr &Jump,
      const TailMetrics &Metrics, const MachineBranchProbabilityInfo &MBPI,
      const AVMInstrInfo &TII);

  static void clearCopiedLivenessFlags(
      MachineBasicBlock &Predecessor,
      const SmallPtrSetImpl<const MachineInstr *> &OriginalInstructions);

  static void debugDecision(const MachineBasicBlock &Predecessor,
                            const MachineBasicBlock &Tail,
                            BranchProbability EdgeProbability,
                            const TailMetrics *Metrics,
                            const Profitability *Profit, StringRef Reason,
                            bool Accepted);
};

} // namespace

bool AVMTailDuplication::isExplicitUnconditionalBranch(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case AVM::JMP_PSEUDO:
  case AVM::RELAX_JMP:
  case AVM::JMP8:
  case AVM::JMP16:
  case AVM::JMPF:
    return true;
  default:
    return false;
  }
}

bool AVMTailDuplication::isConditionalBranch(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case AVM::BR_CC_PSEUDO:
  case AVM::RELAX_BR_EQ:
  case AVM::RELAX_BR_NE:
  case AVM::RELAX_BR_ULT:
  case AVM::RELAX_BR_UGE:
  case AVM::RELAX_BR_SLT:
  case AVM::RELAX_BR_SGE:
  case AVM::BREQ8:
  case AVM::BRNE8:
  case AVM::BRULT8:
  case AVM::BRUGE8:
  case AVM::BRSLT8:
  case AVM::BRSGE8:
  case AVM::BREQ16:
  case AVM::BRNE16:
  case AVM::BRULT16:
  case AVM::BRUGE16:
  case AVM::BRSLT16:
  case AVM::BRSGE16:
    return true;
  default:
    return false;
  }
}

bool AVMTailDuplication::isBranchConditionSetup(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case AVM::CMP16_PSEUDO:
  case AVM::CMP32_PSEUDO:
  case AVM::CMPIS8_PSEUDO:
  case AVM::TST8_PSEUDO:
  case AVM::TST16_PSEUDO:
    return true;
  default:
    return false;
  }
}

bool AVMTailDuplication::isAllowedUnmodeledPseudo(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case AVM::BR_CC_PSEUDO:
  case AVM::JMP_PSEUDO:
  case AVM::ADD16_PSEUDO:
  case AVM::SUB16_PSEUDO:
  case AVM::AND16_PSEUDO:
  case AVM::OR16_PSEUDO:
  case AVM::XOR16_PSEUDO:
  case AVM::ADD32_PSEUDO:
  case AVM::SUB32_PSEUDO:
  case AVM::AND32_PSEUDO:
  case AVM::OR32_PSEUDO:
  case AVM::XOR32_PSEUDO:
  case AVM::PROG_ADD_PSEUDO:
  case AVM::ADDIS8_PSEUDO:
  case AVM::INC16_PSEUDO:
  case AVM::DEC16_PSEUDO:
  case AVM::SHL16_SMALL_PSEUDO:
  case AVM::LSL16I_PSEUDO:
  case AVM::LSR16I_PSEUDO:
  case AVM::ASR16I_PSEUDO:
  case AVM::SHL16V_PSEUDO:
  case AVM::LSR16V_PSEUDO:
  case AVM::ASR16V_PSEUDO:
  case AVM::CMP16_PSEUDO:
  case AVM::CMP32_PSEUDO:
  case AVM::CMPIS8_PSEUDO:
  case AVM::TST8_PSEUDO:
  case AVM::TST16_PSEUDO:
  case AVM::ZEXT8_PSEUDO:
  case AVM::SEXT8_PSEUDO:
  case AVM::COPY16_PSEUDO:
  case AVM::COPY32_PSEUDO:
  case AVM::LDI8_PSEUDO:
  case AVM::LDI16_PSEUDO:
    return true;
  default:
    return false;
  }
}

const char *AVMTailDuplication::jumpKindName(JumpKind Kind) {
  switch (Kind) {
  case JumpKind::Short:
    return "JMP8";
  case JumpKind::Long:
    return "JMP16";
  case JumpKind::Far:
    return "JMPF";
  }
  llvm_unreachable("invalid AVM jump kind");
}

unsigned AVMTailDuplication::weightedCycles(unsigned Cycles,
                                            BranchProbability P) {
  const uint64_t N = P.getNumerator();
  const uint64_t D = P.getDenominator();
  return static_cast<unsigned>((static_cast<uint64_t>(Cycles) * N + D / 2) / D);
}

uint64_t AVMTailDuplication::instructionOffset(const MachineBasicBlock &MBB,
                                               const MachineInstr &Target,
                                               const AVMInstrInfo &TII,
                                               uint64_t BlockStart) {
  uint64_t Offset = BlockStart;
  for (const MachineInstr &MI : MBB) {
    if (&MI == &Target)
      return Offset;
    if (!MI.isDebugInstr() && !MI.isMetaInstruction())
      Offset += TII.getInstSizeInBytes(MI);
  }
  llvm_unreachable("instruction is not in its claimed basic block");
}

BlockOffsets AVMTailDuplication::computeBlockOffsets(const MachineFunction &MF,
                                                     const AVMInstrInfo &TII) {
  BlockOffsets Result;
  uint64_t Offset = 0;
  for (const MachineBasicBlock &MBB : MF) {
    Result.Starts[&MBB] = Offset;
    for (const MachineInstr &MI : MBB)
      if (!MI.isDebugInstr() && !MI.isMetaInstruction())
        Offset += TII.getInstSizeInBytes(MI);
  }
  return Result;
}

JumpKind AVMTailDuplication::jumpKind(uint64_t Source, uint64_t Target,
                                      const MachineInstr *MI) {
  if (MI) {
    if (MI->getOpcode() == AVM::JMP8)
      return JumpKind::Short;
    if (MI->getOpcode() == AVM::JMP16)
      return JumpKind::Long;
    if (MI->getOpcode() == AVM::JMPF)
      return JumpKind::Far;
  }

  int64_t ShortDistance =
      static_cast<int64_t>(Target) - static_cast<int64_t>(Source + 2);
  if (isInt<8>(ShortDistance))
    return JumpKind::Short;
  int64_t LongDistance =
      static_cast<int64_t>(Target) - static_cast<int64_t>(Source + 3);
  return isInt<16>(LongDistance) ? JumpKind::Long : JumpKind::Far;
}

unsigned AVMTailDuplication::jumpCycles(JumpKind Kind) {
  switch (Kind) {
  case JumpKind::Short:
    return AVM::getFixedCycles(AVM::AVMCostKind::Jmp8);
  case JumpKind::Long:
    return AVM::getFixedCycles(AVM::AVMCostKind::Jmp16);
  case JumpKind::Far:
    return AVM::getFixedCycles(AVM::AVMCostKind::JmpFar);
  }
  llvm_unreachable("invalid AVM jump kind");
}

unsigned
AVMTailDuplication::conditionalCycles(uint64_t Source, uint64_t Target,
                                      BranchProbability TakenProbability) {
  int64_t ShortDistance =
      static_cast<int64_t>(Target) - static_cast<int64_t>(Source + 2);
  AVM::AVMCostKind Kind = isInt<8>(ShortDistance) ? AVM::AVMCostKind::BrEq8
                                                  : AVM::AVMCostKind::BrEq16;
  return AVM::getExpectedBranchCycles(Kind, TakenProbability);
}

std::optional<TailMetrics> AVMTailDuplication::checkTailLegality(
    const MachineBasicBlock &Tail, const AVMInstrInfo &TII,
    unsigned MaxInstructions, unsigned MaxBytes, StringRef &Reason) {
  if (Tail.pred_size() < 2) {
    Reason = "tail has fewer than two predecessors";
    return std::nullopt;
  }
  if (Tail.succ_size() < 1 || Tail.succ_size() > 2) {
    Reason = "tail does not have one or two successors";
    return std::nullopt;
  }
  if (Tail.isEHPad()) {
    Reason = "tail is an exception-handling pad";
    return std::nullopt;
  }
  if (Tail.hasAddressTaken()) {
    Reason = "tail address is taken";
    return std::nullopt;
  }

  TailMetrics Metrics;
  for (const MachineInstr &MI : Tail) {
    if (MI.isDebugInstr())
      continue;
    if (MI.isMetaInstruction()) {
      Reason = "tail contains a meta instruction";
      return std::nullopt;
    }
    if (MI.isPHI()) {
      Reason = "tail contains a PHI";
      return std::nullopt;
    }
    if (MI.isCall() || MI.isReturn() || MI.isIndirectBranch()) {
      Reason = "tail contains call, return, or indirect control flow";
      return std::nullopt;
    }
    if (MI.isInlineAsm() || MI.isConvergent() || MI.isNotDuplicable()) {
      Reason = "tail contains a non-duplicatable instruction";
      return std::nullopt;
    }
    if (MI.hasUnmodeledSideEffects() && !isAllowedUnmodeledPseudo(MI)) {
      Reason = "tail contains unmodeled side effects";
      return std::nullopt;
    }
    if (MI.mayLoadOrStore()) {
      Reason = "tail contains a load or store";
      return std::nullopt;
    }
    if (MI.isBundledWithPred() || MI.isBundledWithSucc()) {
      Reason = "tail contains a bundled instruction";
      return std::nullopt;
    }
    for (const MachineOperand &MO : MI.operands()) {
      if (MO.isRegMask()) {
        Reason = "tail contains a register-mask operand";
        return std::nullopt;
      }
      if (MO.isReg() && MO.getReg() && !MO.getReg().isPhysical()) {
        Reason = "tail contains a virtual register";
        return std::nullopt;
      }
      if (!MO.isReg() && !MO.isImm() && !MO.isMBB()) {
        Reason = "tail contains an unsupported operand kind";
        return std::nullopt;
      }
    }

    unsigned Bytes = TII.getInstSizeInBytes(MI);
    Metrics.DuplicatedBytes += Bytes;
    if (MI.isBranch()) {
      Metrics.HasExplicitUnconditionalBranch |=
          isExplicitUnconditionalBranch(MI);
      continue;
    }
    // Comparisons and tests are part of the copied branch sequence.  Count
    // only the payload instructions against the non-branch safety limit, but
    // retain their bytes when locating and range-checking the branch.
    if (!isBranchConditionSetup(MI))
      ++Metrics.NonBranchInstructions;
    Metrics.NonBranchBytes += Bytes;
  }

  if (Metrics.NonBranchInstructions > MaxInstructions) {
    Reason = "non-branch instruction limit exceeded";
    return std::nullopt;
  }
  if (Metrics.DuplicatedBytes > MaxBytes) {
    Reason = "encoded-byte limit exceeded";
    return std::nullopt;
  }
  return Metrics;
}

unsigned AVMTailDuplication::transferCost(
    const MachineBasicBlock &Tail, const MachineBasicBlock &Placement,
    uint64_t BranchOffset, const BlockOffsets &Offsets,
    const MachineBranchProbabilityInfo &MBPI, const AVMInstrInfo &TII,
    bool IsDuplicated) {
  MachineBasicBlock *TBB = nullptr;
  MachineBasicBlock *FBB = nullptr;
  SmallVector<MachineOperand, 1> Cond;
  if (TII.analyzeBranch(const_cast<MachineBasicBlock &>(Tail), TBB, FBB, Cond))
    return std::numeric_limits<unsigned>::max();

  const MachineBasicBlock *LayoutSuccessor = Placement.getNextNode();
  if (Tail.succ_size() == 1) {
    MachineBasicBlock *Successor = *Tail.succ_begin();
    if (LayoutSuccessor == Successor)
      return 0;
    bool HasExplicitJump = false;
    if (MachineBasicBlock::const_iterator I = Tail.getLastNonDebugInstr();
        I != Tail.end())
      HasExplicitJump = isExplicitUnconditionalBranch(*I);
    if (!HasExplicitJump && !IsDuplicated)
      return 0;
    return jumpCycles(jumpKind(BranchOffset, Offsets.adjusted(*Successor)));
  }

  MachineBasicBlock *First = *Tail.succ_begin();
  MachineBasicBlock *Second = *std::next(Tail.succ_begin());
  auto EdgeProbability = [&](MachineBasicBlock *Succ) {
    return MBPI.getEdgeProbability(&Tail, Succ);
  };

  auto ConditionalTo = [&](MachineBasicBlock *Target) {
    return conditionalCycles(BranchOffset, Offsets.adjusted(*Target),
                             EdgeProbability(Target));
  };

  if (LayoutSuccessor == First)
    return ConditionalTo(Second);
  if (LayoutSuccessor == Second)
    return ConditionalTo(First);

  auto CostWithJump = [&](MachineBasicBlock *ConditionalTarget,
                          MachineBasicBlock *JumpTarget) {
    BranchProbability Taken = EdgeProbability(ConditionalTarget);
    unsigned Conditional = ConditionalTo(ConditionalTarget);
    uint64_t JumpOffset = BranchOffset + 3;
    unsigned Jump =
        jumpCycles(jumpKind(JumpOffset, Offsets.adjusted(*JumpTarget)));
    return Conditional + weightedCycles(Jump, Taken.getCompl());
  };
  return std::min(CostWithJump(First, Second), CostWithJump(Second, First));
}

bool AVMTailDuplication::branchRangeDegrades(
    const MachineFunction &MF, const MachineBasicBlock &Predecessor,
    const MachineBasicBlock &Tail, const BlockOffsets &OldOffsets,
    const BlockOffsets &NewOffsets, const AVMInstrInfo &TII) {
  auto Rank = [](JumpKind Kind) {
    switch (Kind) {
    case JumpKind::Short:
      return 0;
    case JumpKind::Long:
      return 1;
    case JumpKind::Far:
      return 2;
    }
    llvm_unreachable("invalid AVM jump kind");
  };

  for (const MachineBasicBlock &MBB : MF) {
    if (&MBB == &Predecessor || &MBB == &Tail)
      continue;
    for (const MachineInstr &MI : MBB) {
      if (!isConditionalBranch(MI) && !isExplicitUnconditionalBranch(MI))
        continue;
      if (MI.getNumExplicitOperands() < 1 || !MI.getOperand(0).isMBB())
        continue;

      const MachineBasicBlock *Target = MI.getOperand(0).getMBB();
      uint64_t OldSource =
          instructionOffset(MBB, MI, TII, OldOffsets.Starts.lookup(&MBB));
      uint64_t NewSource =
          instructionOffset(MBB, MI, TII, NewOffsets.Starts.lookup(&MBB));
      uint64_t OldTarget = OldOffsets.Starts.lookup(Target);
      uint64_t NewTarget = NewOffsets.Starts.lookup(Target);
      if (isExplicitUnconditionalBranch(MI)) {
        if (Rank(jumpKind(NewSource, NewTarget, &MI)) >
            Rank(jumpKind(OldSource, OldTarget, &MI)))
          return true;
        continue;
      }

      int64_t OldDistance =
          static_cast<int64_t>(OldTarget) - static_cast<int64_t>(OldSource + 2);
      int64_t NewDistance =
          static_cast<int64_t>(NewTarget) - static_cast<int64_t>(NewSource + 2);
      if (isInt<8>(OldDistance) && !isInt<8>(NewDistance))
        return true;
    }
  }
  return false;
}

bool AVMTailDuplication::tryPlaceTailAsFallthrough(
    MachineFunction &MF, MachineBasicBlock &Predecessor,
    MachineBasicBlock &Tail, MachineInstr &Jump,
    const MachineBranchProbabilityInfo &MBPI,
    const MachineBlockFrequencyInfo &MBFI, const AVMInstrInfo &TII,
    double &OriginalScore, double &PlacedScore, StringRef &Reason) {
  MachineBasicBlock *AfterPredecessor = Predecessor.getNextNode();
  if (!AfterPredecessor || AfterPredecessor == &Tail) {
    Reason = "tail is already the predecessor fallthrough";
    return false;
  }
  if (!Tail.isSuccessor(AfterPredecessor)) {
    Reason = "moving the tail would require a new tail jump";
    return false;
  }

  MachineBasicBlock *BeforeTail = Tail.getPrevNode();
  if (BeforeTail && BeforeTail->isSuccessor(&Tail)) {
    Reason = "moving the tail would break an existing fallthrough";
    return false;
  }

  MachineBasicBlock *OldTailTarget = nullptr;
  MachineBasicBlock *OldTailFallthrough = nullptr;
  SmallVector<MachineOperand, 4> TailCondition;
  if (TII.analyzeBranch(Tail, OldTailTarget, OldTailFallthrough,
                        TailCondition)) {
    Reason = "tail branch sequence is not analyzable";
    return false;
  }

  MachineBasicBlock *NewTailTarget = nullptr;
  if (Tail.succ_size() == 2) {
    for (MachineBasicBlock *Successor : Tail.successors())
      if (Successor != AfterPredecessor)
        NewTailTarget = Successor;
    if (!NewTailTarget || TailCondition.empty()) {
      Reason = "tail branch cannot be rebuilt for the new fallthrough";
      return false;
    }
    if (NewTailTarget != OldTailTarget &&
        TII.reverseBranchCondition(TailCondition)) {
      Reason = "tail condition cannot be reversed for the new fallthrough";
      return false;
    }
  }

  MachineBasicBlock *OriginalNext = Tail.getNextNode();
  BlockOffsets OldOffsets = computeBlockOffsets(MF, TII);
  uint64_t JumpOffset = instructionOffset(
      Predecessor, Jump, TII, OldOffsets.Starts.lookup(&Predecessor));
  uint64_t TailBranchOffset = OldOffsets.Starts.lookup(&Tail);
  for (const MachineInstr &MI : Tail) {
    if (MI.isDebugInstr() || MI.isMetaInstruction())
      continue;
    if (MI.isBranch())
      break;
    TailBranchOffset += TII.getInstSizeInBytes(MI);
  }
  unsigned OldTailCost =
      transferCost(Tail, Tail, TailBranchOffset, OldOffsets, MBPI, TII, false);
  if (OldTailCost == std::numeric_limits<unsigned>::max()) {
    Reason = "tail transfer cost is not analyzable";
    return false;
  }
  unsigned JumpCost =
      jumpCycles(jumpKind(JumpOffset, OldOffsets.Starts.lookup(&Tail), &Jump));
  OriginalScore = JumpCost + OldTailCost;

  MF.splice(std::next(Predecessor.getIterator()), Tail.getIterator());
  BlockOffsets NewOffsets = computeBlockOffsets(MF, TII);
  uint64_t NewTailBranchOffset = NewOffsets.Starts.lookup(&Tail);
  for (const MachineInstr &MI : Tail) {
    if (MI.isDebugInstr() || MI.isMetaInstruction())
      continue;
    if (MI.isBranch())
      break;
    NewTailBranchOffset += TII.getInstSizeInBytes(MI);
  }
  PlacedScore = transferCost(Tail, Tail, NewTailBranchOffset, NewOffsets, MBPI,
                             TII, false);
  BranchProbability PToTail =
      MBPI.getEdgeProbability(&Predecessor, &Tail);
  double EdgeFrequency =
      static_cast<double>(
          MBFI.getBlockFreq(&Predecessor).getFrequency()) *
      static_cast<double>(PToTail.getNumerator()) /
      static_cast<double>(PToTail.getDenominator());
  double ExpectedJumpSaving = EdgeFrequency * JumpCost;
  double ExpectedTailPenalty =
      static_cast<double>(MBFI.getBlockFreq(&Tail).getFrequency()) *
      (PlacedScore > OldTailCost ? PlacedScore - OldTailCost : 0.0);
  // Static block frequencies can understate a profitable inner-loop edge.  A
  // small margin admits those cases while rejecting placements whose shared
  // tail becomes materially more expensive for its other predecessors.
  bool FrequencyModelRejects =
      ExpectedTailPenalty > ExpectedJumpSaving * 1.25;
  bool RangeDegrades =
      branchRangeDegrades(MF, Predecessor, Tail, OldOffsets, NewOffsets, TII);
  if (PlacedScore >= OriginalScore || FrequencyModelRejects || RangeDegrades) {
    if (OriginalNext)
      MF.splice(OriginalNext->getIterator(), Tail.getIterator());
    else
      MF.splice(MF.end(), Tail.getIterator());
    if (PlacedScore >= OriginalScore)
      Reason = "fallthrough placement does not reduce expected transfer cycles";
    else if (FrequencyModelRejects)
      Reason = "block-frequency model predicts a transfer-cycle regression";
    else
      Reason = "fallthrough placement degrades another branch range";
    return false;
  }

  Jump.eraseFromParent();
  TII.removeBranch(Tail);
  if (NewTailTarget)
    TII.insertBranch(Tail, NewTailTarget, nullptr, TailCondition, DebugLoc());
  Reason = "fallthrough placement reduces expected transfer cycles";
  return true;
}

unsigned AVMTailDuplication::branchRangePenalty(
    const MachineFunction &MF, const MachineBasicBlock &Predecessor,
    const MachineBasicBlock &Tail, const BlockOffsets &Offsets,
    const MachineBranchProbabilityInfo &MBPI, const AVMInstrInfo &TII) {
  unsigned Penalty = 0;
  for (const MachineBasicBlock &MBB : MF) {
    if (&MBB == &Predecessor || &MBB == &Tail)
      continue;
    for (const MachineInstr &MI : MBB) {
      if (!isConditionalBranch(MI) && !isExplicitUnconditionalBranch(MI))
        continue;
      if (MI.getNumExplicitOperands() < 1 || !MI.getOperand(0).isMBB())
        continue;
      const MachineBasicBlock *Target = MI.getOperand(0).getMBB();
      uint64_t Source =
          instructionOffset(MBB, MI, TII, Offsets.Starts.lookup(&MBB));
      uint64_t NewSource = Offsets.adjusted(Source);
      uint64_t OldTarget = Offsets.Starts.lookup(Target);
      uint64_t NewTarget = Offsets.adjusted(*Target);

      if (isExplicitUnconditionalBranch(MI)) {
        unsigned OldCycles = jumpCycles(jumpKind(Source, OldTarget, &MI));
        unsigned NewCycles = jumpCycles(jumpKind(NewSource, NewTarget, &MI));
        if (NewCycles > OldCycles)
          Penalty += NewCycles - OldCycles;
        continue;
      }

      BranchProbability Taken = MBB.isSuccessor(Target)
                                    ? MBPI.getEdgeProbability(&MBB, Target)
                                    : BranchProbability(1, 2);
      unsigned OldCycles = conditionalCycles(Source, OldTarget, Taken);
      unsigned NewCycles = conditionalCycles(NewSource, NewTarget, Taken);
      if (NewCycles > OldCycles)
        Penalty += NewCycles - OldCycles;
    }
  }
  return Penalty;
}

Profitability AVMTailDuplication::estimateProfitability(
    const MachineFunction &MF, const MachineBasicBlock &Predecessor,
    const MachineBasicBlock &Tail, const MachineInstr &Jump,
    const TailMetrics &Metrics, const MachineBranchProbabilityInfo &MBPI,
    const AVMInstrInfo &TII) {
  BlockOffsets Offsets = computeBlockOffsets(MF, TII);
  uint64_t PredecessorStart = Offsets.Starts.lookup(&Predecessor);
  uint64_t JumpOffset =
      instructionOffset(Predecessor, Jump, TII, PredecessorStart);
  uint64_t TailStart = Offsets.Starts.lookup(&Tail);

  Profitability Result;
  Result.EliminatedJump = jumpKind(JumpOffset, TailStart, &Jump);
  unsigned EliminatedCycles = jumpCycles(Result.EliminatedJump);

  uint64_t TailBranchOffset = TailStart + Metrics.NonBranchBytes;
  unsigned OriginalTransfer =
      transferCost(Tail, Tail, TailBranchOffset, Offsets, MBPI, TII, false);

  unsigned RemovedBytes = TII.getInstSizeInBytes(Jump);
  Offsets.InsertionEnd = PredecessorStart;
  for (const MachineInstr &MI : Predecessor)
    if (!MI.isDebugInstr() && !MI.isMetaInstruction())
      Offsets.InsertionEnd += TII.getInstSizeInBytes(MI);
  if (Metrics.DuplicatedBytes > RemovedBytes)
    Offsets.Growth = Metrics.DuplicatedBytes - RemovedBytes;

  uint64_t CopiedBranchOffset = JumpOffset + Metrics.NonBranchBytes;
  unsigned DuplicatedTransfer = transferCost(
      Tail, Predecessor, CopiedBranchOffset, Offsets, MBPI, TII, true);

  if (OriginalTransfer == std::numeric_limits<unsigned>::max() ||
      DuplicatedTransfer == std::numeric_limits<unsigned>::max()) {
    Result.OriginalCycles = 0;
    Result.DuplicatedCycles = std::numeric_limits<unsigned>::max();
    return Result;
  }

  Result.RangePenalty =
      branchRangePenalty(MF, Predecessor, Tail, Offsets, MBPI, TII);
  Result.OriginalCycles = EliminatedCycles + OriginalTransfer;
  Result.DuplicatedCycles = DuplicatedTransfer + Result.RangePenalty;
  return Result;
}

void AVMTailDuplication::clearCopiedLivenessFlags(
    MachineBasicBlock &Predecessor,
    const SmallPtrSetImpl<const MachineInstr *> &OriginalInstructions) {
  for (MachineInstr &MI : Predecessor) {
    if (OriginalInstructions.contains(&MI))
      continue;
    for (MachineOperand &MO : MI.operands()) {
      if (!MO.isReg())
        continue;
      if (MO.isUse())
        MO.setIsKill(false);
      if (MO.isDef())
        MO.setIsDead(false);
    }
  }
}

void AVMTailDuplication::debugDecision(const MachineBasicBlock &Predecessor,
                                       const MachineBasicBlock &Tail,
                                       BranchProbability EdgeProbability,
                                       const TailMetrics *Metrics,
                                       const Profitability *Profit,
                                       StringRef Reason, bool Accepted) {
  LLVM_DEBUG({
    dbgs() << "AVM tail duplication: predecessor "
           << printMBBReference(Predecessor) << ", tail "
           << printMBBReference(Tail) << ", edge probability "
           << EdgeProbability << '\n';
    if (Profit)
      dbgs() << "  eliminated jump " << jumpKindName(Profit->EliminatedJump)
             << ", estimated original cycles " << Profit->OriginalCycles
             << ", estimated duplicated cycles " << Profit->DuplicatedCycles
             << ", range penalty " << Profit->RangePenalty << '\n';
    if (Metrics)
      dbgs() << "  duplicated instruction count "
             << Metrics->NonBranchInstructions << ", duplicated byte count "
             << Metrics->DuplicatedBytes << '\n';
    dbgs() << "  " << (Accepted ? "accept" : "reject") << ": " << Reason
           << '\n';
  });
}

bool AVMTailDuplication::runOnMachineFunction(MachineFunction &MF) {
  if (!EnableAVMTailDuplication || OptLevel == CodeGenOptLevel::None ||
      MF.getFunction().hasOptNone())
    return false;

  const AVMInstrInfo &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
  const MachineBranchProbabilityInfo &MBPI =
      getAnalysis<MachineBranchProbabilityInfoWrapperPass>().getMBPI();
  MachineBlockFrequencyInfo &MBFI =
      getAnalysis<MachineBlockFrequencyInfoWrapperPass>().getMBFI();

  bool SizeOriented = MF.getFunction().hasOptSize() ||
                      MF.getFunction().hasMinSize() ||
                      OptLevel == CodeGenOptLevel::Less;
  unsigned MaxInstructions =
      AVMTailDupMaxInsts ? AVMTailDupMaxInsts : (SizeOriented ? 3U : 6U);
  unsigned MaxBytes =
      AVMTailDupMaxBytes ? AVMTailDupMaxBytes : (SizeOriented ? 10U : 18U);

  SmallVector<Candidate, 16> Candidates;
  SmallPtrSet<const MachineBasicBlock *, 16> SeenPredecessors;
  for (MachineBasicBlock &Predecessor : MF) {
    if ((Predecessor.succ_size() != 1 && Predecessor.succ_size() != 2) ||
        Predecessor.hasAddressTaken() || Predecessor.isEHPad())
      continue;
    MachineBasicBlock::iterator Last = Predecessor.getLastNonDebugInstr();
    if (Last == Predecessor.end() || !isExplicitUnconditionalBranch(*Last) ||
        Last->getNumExplicitOperands() != 1 || !Last->getOperand(0).isMBB())
      continue;
    MachineBasicBlock *Tail = Last->getOperand(0).getMBB();
    if (Tail == &Predecessor || !Predecessor.isSuccessor(Tail))
      continue;
    Candidates.push_back({&Predecessor, Tail});
  }

  TailDuplicator Duplicator;
  MBFIWrapper MBFIW(MBFI);
  Duplicator.initMF(MF, /*PreRegAlloc=*/false, &MBPI, &MBFIW,
                    /*PSI=*/nullptr, /*LayoutMode=*/false);

  bool Changed = false;
  for (const Candidate &C : Candidates) {
    MachineBasicBlock &Predecessor = *C.Predecessor;
    MachineBasicBlock &Tail = *C.Tail;
    if (!SeenPredecessors.insert(&Predecessor).second ||
        (Predecessor.succ_size() != 1 && Predecessor.succ_size() != 2) ||
        !Predecessor.isSuccessor(&Tail))
      continue;

    BranchProbability EdgeProbability =
        MBPI.getEdgeProbability(&Predecessor, &Tail);
    if (Predecessor.hasAddressTaken() || Tail.hasAddressTaken()) {
      debugDecision(Predecessor, Tail, EdgeProbability, nullptr, nullptr,
                    "predecessor or tail address is taken", false);
      continue;
    }

    MachineBasicBlock::iterator Jump = Predecessor.getLastNonDebugInstr();
    // An earlier candidate can move this predecessor or rewrite its
    // terminators.  Revalidate the cached edge before dereferencing the
    // terminator or applying profitability estimates that assume an
    // unconditional jump.
    if (Jump == Predecessor.end() || !isExplicitUnconditionalBranch(*Jump) ||
        Jump->getNumExplicitOperands() != 1 || !Jump->getOperand(0).isMBB() ||
        Jump->getOperand(0).getMBB() != &Tail) {
      debugDecision(Predecessor, Tail, EdgeProbability, nullptr, nullptr,
                    "candidate no longer ends in the recorded jump", false);
      continue;
    }

    double OriginalPlacementScore = 0.0;
    double PlacedScore = 0.0;
    StringRef PlacementReason;
    if (tryPlaceTailAsFallthrough(MF, Predecessor, Tail, *Jump, MBPI, MBFI, TII,
                                  OriginalPlacementScore, PlacedScore,
                                  PlacementReason)) {
      LLVM_DEBUG(dbgs() << "AVM tail placement: predecessor "
                        << printMBBReference(Predecessor) << ", tail "
                        << printMBBReference(Tail) << ", edge probability "
                        << EdgeProbability << ", estimated original score "
                        << OriginalPlacementScore << ", estimated placed score "
                        << PlacedScore << ", accept: " << PlacementReason
                        << '\n');
      Changed = true;
      continue;
    }

    StringRef Reason;
    std::optional<TailMetrics> Metrics =
        checkTailLegality(Tail, TII, MaxInstructions, MaxBytes, Reason);
    if (!Metrics) {
      debugDecision(Predecessor, Tail, EdgeProbability, nullptr, nullptr,
                    Reason, false);
      continue;
    }

    Profitability Profit = estimateProfitability(MF, Predecessor, Tail, *Jump,
                                                 *Metrics, MBPI, TII);
    if (Profit.DuplicatedCycles == std::numeric_limits<unsigned>::max()) {
      debugDecision(Predecessor, Tail, EdgeProbability, &*Metrics, &Profit,
                    "tail branch sequence is not analyzable", false);
      continue;
    }
    if (Profit.savings() <= 0) {
      debugDecision(Predecessor, Tail, EdgeProbability, &*Metrics, &Profit,
                    "estimated cycle saving is not positive", false);
      continue;
    }
    MachineBasicBlock *DuplicationPredecessor = &Predecessor;
    MachineBasicBlock *EdgeBlock = nullptr;
    if (Predecessor.succ_size() == 2) {
      EdgeBlock = MF.CreateMachineBasicBlock(Predecessor.getBasicBlock());
      MF.insert(std::next(Predecessor.getIterator()), EdgeBlock);
      EdgeBlock->splice(EdgeBlock->end(), &Predecessor, Jump->getIterator());
      Predecessor.replaceSuccessor(&Tail, EdgeBlock);
      EdgeBlock->addSuccessor(&Tail, BranchProbability::getOne());
      DuplicationPredecessor = EdgeBlock;
    }

    if (!Duplicator.canTailDuplicate(&Tail, DuplicationPredecessor)) {
      if (EdgeBlock) {
        Predecessor.splice(Predecessor.end(), EdgeBlock,
                           EdgeBlock->begin()->getIterator());
        EdgeBlock->removeSuccessor(&Tail);
        Predecessor.replaceSuccessor(EdgeBlock, &Tail);
        MF.erase(EdgeBlock);
      }
      debugDecision(Predecessor, Tail, EdgeProbability, &*Metrics, &Profit,
                    "generic TailDuplicator rejected the edge", false);
      continue;
    }

    SmallPtrSet<const MachineInstr *, 16> OriginalInstructions;
    for (const MachineInstr &MI : *DuplicationPredecessor)
      // TailDuplicator removes the predecessor's branch instructions before
      // allocating the clones.  MachineFunction recycles removed
      // MachineInstr storage, so retaining a removed branch pointer could make
      // the first clone look like an original instruction.
      if (!MI.isBranch())
        OriginalInstructions.insert(&MI);
    // isLiveIn returns true for any overlapping lane, not only when the
    // requested mask is fully covered.  Add every tail mask and canonicalize
    // the vector so partially overlapping masks are unioned.
    for (const auto &LiveIn : Tail.liveins())
      DuplicationPredecessor->addLiveIn(LiveIn.PhysReg, LiveIn.LaneMask);
    DuplicationPredecessor->sortUniqueLiveIns();

    SmallVector<MachineBasicBlock *, 1> Selected{DuplicationPredecessor};
    SmallVector<MachineBasicBlock *, 1> DuplicatedPredecessors;
    // Supplying Tail itself as the forced layout predecessor prevents the
    // utility's unrelated "merge the final predecessor" cleanup.  The AVM
    // transformation deliberately retains the shared original tail.  With
    // this single selected predecessor, successful canTailDuplicate above
    // guarantees that tailDuplicate visits the predecessor, records it in
    // TDBBs, and sets Changed.  The post-RA path has no later failure, and Tail
    // is neither the selected predecessor nor a CFG predecessor eligible for
    // the final merge.
    bool Duplicated = Duplicator.tailDuplicateAndUpdate(
        /*IsSimple=*/false, &Tail, /*ForcedLayoutPred=*/&Tail,
        &DuplicatedPredecessors, /*RemovalCallback=*/nullptr, &Selected);
    if (!Duplicated ||
        !is_contained(DuplicatedPredecessors, DuplicationPredecessor))
      llvm_unreachable(
          "prevalidated AVM tail-duplication candidate was not duplicated");

    clearCopiedLivenessFlags(*DuplicationPredecessor, OriginalInstructions);
    debugDecision(Predecessor, Tail, EdgeProbability, &*Metrics, &Profit,
                  "positive expected cycle saving", true);
    Changed = true;
  }
  return Changed;
}

char AVMTailDuplication::ID = 0;

INITIALIZE_PASS_BEGIN(AVMTailDuplication, DEBUG_TYPE, PASS_NAME, false, false)
INITIALIZE_PASS_DEPENDENCY(MachineBranchProbabilityInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(MachineBlockFrequencyInfoWrapperPass)
INITIALIZE_PASS_END(AVMTailDuplication, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMTailDuplicationPass(CodeGenOptLevel OptLevel) {
  return new AVMTailDuplication(OptLevel);
}

void llvm::registerAVMTailDuplicationOptions() {
  // opt's legacy PassNameParser owns an option with the pass argument for
  // every initialized pass.  Drivers such as clang and llc do not.  Register
  // the requested A/B switch only when that spelling is still available.
  if (cl::getRegisteredOptions().count("avm-tail-duplication"))
    return;
  static cl::opt<bool, true> EnableOption(
      "avm-tail-duplication", cl::location(EnableAVMTailDuplication),
      cl::Hidden, cl::desc("Enable late AVM tail duplication"), cl::init(true));
  (void)EnableOption;
}
