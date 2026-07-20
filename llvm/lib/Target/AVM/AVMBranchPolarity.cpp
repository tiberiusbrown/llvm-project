//===-- AVMBranchPolarity.cpp - Choose AVM branch polarity ---------------===//

#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/MachineBranchProbabilityInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "avm-branch-polarity"
#define PASS_NAME "AVM measured branch polarity"

namespace {
class AVMBranchPolarity final : public MachineFunctionPass {
public:
  static char ID;
  AVMBranchPolarity() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return PASS_NAME; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineBranchProbabilityInfoWrapperPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    const AVMInstrInfo &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
    const MachineBranchProbabilityInfo &MBPI =
        getAnalysis<MachineBranchProbabilityInfoWrapperPass>().getMBPI();

    DenseMap<const MachineBasicBlock *, uint64_t> BlockOffsets;
    uint64_t Offset = 0;
    for (const MachineBasicBlock &MBB : MF) {
      BlockOffsets[&MBB] = Offset;
      for (const MachineInstr &MI : MBB) {
        if (MI.getOpcode() == AVM::BR_CC_PSEUDO)
          Offset += 6;
        else if (MI.getOpcode() == AVM::JMP_PSEUDO)
          Offset += 4;
        else
          Offset += TII.getInstSizeInBytes(MI);
      }
    }

    auto IsRel8 = [&](const MachineBasicBlock &MBB,
                      const MachineBasicBlock &Target) {
      uint64_t BranchOffset = BlockOffsets.lookup(&MBB);
      for (const MachineInstr &MI : MBB) {
        if (MI.getOpcode() == AVM::BR_CC_PSEUDO ||
            MI.getOpcode() == AVM::RELAX_BR_EQ ||
            MI.getOpcode() == AVM::RELAX_BR_NE ||
            MI.getOpcode() == AVM::RELAX_BR_ULT ||
            MI.getOpcode() == AVM::RELAX_BR_UGE ||
            MI.getOpcode() == AVM::RELAX_BR_SLT ||
            MI.getOpcode() == AVM::RELAX_BR_SGE)
          break;
        BranchOffset += TII.getInstSizeInBytes(MI);
      }
      int64_t Distance = static_cast<int64_t>(BlockOffsets.lookup(&Target)) -
                         static_cast<int64_t>(BranchOffset);
      return isInt<8>(Distance);
    };

    auto ExpectedCycles = [&](const MachineBasicBlock &MBB,
                              const MachineBasicBlock &Target,
                              BranchProbability TakenProbability) {
      AVM::AVMCostKind Kind = IsRel8(MBB, Target) ? AVM::AVMCostKind::BrEq8
                                                  : AVM::AVMCostKind::BrEq16;
      return AVM::getExpectedBranchCycles(Kind, TakenProbability);
    };

    bool Changed = false;
    for (MachineBasicBlock &MBB : MF) {
      if (MBB.succ_size() != 2)
        continue;

      MachineBasicBlock *TBB = nullptr;
      MachineBasicBlock *FBB = nullptr;
      SmallVector<MachineOperand, 1> Cond;
      if (TII.analyzeBranch(MBB, TBB, FBB, Cond) || !TBB || Cond.empty())
        continue;
      if (!FBB) {
        for (MachineBasicBlock *Succ : MBB.successors())
          if (Succ != TBB) {
            FBB = Succ;
            break;
          }
      }
      if (!FBB || FBB == TBB)
        continue;

      BranchProbability TrueProbability = MBPI.getEdgeProbability(&MBB, TBB);
      BranchProbability FalseProbability = MBPI.getEdgeProbability(&MBB, FBB);
      if (TrueProbability == FalseProbability ||
          FalseProbability > TrueProbability)
        continue;

      unsigned CurrentCost = ExpectedCycles(MBB, *TBB, TrueProbability);
      unsigned ReversedCost = ExpectedCycles(MBB, *FBB, FalseProbability);
      if (ReversedCost >= CurrentCost)
        continue;

      DebugLoc DL;
      if (MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
          I != MBB.end())
        DL = I->getDebugLoc();
      TII.removeBranch(MBB);
      if (TII.reverseBranchCondition(Cond))
        llvm_unreachable("AVM branch condition stopped being reversible");

      MachineBasicBlock *LayoutSuccessor = nullptr;
      if (std::next(MBB.getIterator()) != MF.end())
        LayoutSuccessor = &*std::next(MBB.getIterator());
      MachineBasicBlock *NewFalse = TBB == LayoutSuccessor ? nullptr : TBB;
      TII.insertBranch(MBB, FBB, NewFalse, Cond, DL);
      Changed = true;
    }
    return Changed;
  }
};
} // namespace

char AVMBranchPolarity::ID = 0;

INITIALIZE_PASS_BEGIN(AVMBranchPolarity, DEBUG_TYPE, PASS_NAME, false, false)
INITIALIZE_PASS_DEPENDENCY(MachineBranchProbabilityInfoWrapperPass)
INITIALIZE_PASS_END(AVMBranchPolarity, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMBranchPolarityPass() {
  return new AVMBranchPolarity();
}
