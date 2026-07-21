//===-- AVMFinalControlFlow.cpp - Final AVM control-flow cleanup --------===//

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/InitializePasses.h"
#include <algorithm>
#include <iterator>

using namespace llvm;

#define DEBUG_TYPE "avm-final-control-flow"
#define PASS_NAME "AVM final control-flow cleanup"

namespace {

static bool emitsCode(const MachineBasicBlock &MBB) {
  return llvm::any_of(MBB, [](const MachineInstr &MI) {
    return !MI.isDebugInstr() && !MI.isMetaInstruction();
  });
}

static MachineBasicBlock *getEffectiveLayoutSuccessor(MachineBasicBlock &MBB) {
  MachineFunction &MF = *MBB.getParent();
  for (auto I = std::next(MBB.getIterator()); I != MF.end(); ++I)
    if (emitsCode(*I))
      return &*I;
  return nullptr;
}

static bool isLayoutPredecessor(const MachineBasicBlock &Candidate,
                                const MachineBasicBlock &MBB) {
  const MachineFunction &MF = *MBB.getParent();
  for (auto I = Candidate.getIterator(); I != MF.end(); ++I)
    if (&*I == &MBB)
      return true;
  return false;
}

static unsigned getInverseBranchOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVM::RELAX_BR_EQ:
    return AVM::RELAX_BR_NE;
  case AVM::RELAX_BR_NE:
    return AVM::RELAX_BR_EQ;
  case AVM::RELAX_BR_ULT:
    return AVM::RELAX_BR_UGE;
  case AVM::RELAX_BR_UGE:
    return AVM::RELAX_BR_ULT;
  case AVM::RELAX_BR_SLT:
    return AVM::RELAX_BR_SGE;
  case AVM::RELAX_BR_SGE:
    return AVM::RELAX_BR_SLT;
  default:
    return 0;
  }
}

static SmallVector<MachineInstr *, 2>
getLastCodeInstructions(MachineBasicBlock &MBB) {
  SmallVector<MachineInstr *, 2> Result;
  for (MachineInstr &MI : llvm::reverse(MBB)) {
    if (MI.isDebugInstr() || MI.isMetaInstruction())
      continue;
    Result.push_back(&MI);
    if (Result.size() == 2)
      break;
  }
  std::reverse(Result.begin(), Result.end());
  return Result;
}

static bool isEligibleReturnTail(const MachineBasicBlock &Target,
                                 const AVMInstrInfo &TII, unsigned &TailSize) {
  SmallVector<const MachineInstr *, 2> Instructions;
  for (const MachineInstr &MI : Target) {
    if (MI.isDebugInstr() || MI.isMetaInstruction())
      continue;
    Instructions.push_back(&MI);
  }

  if (Instructions.empty() || Instructions.back()->getOpcode() != AVM::RET)
    return false;

  if (Instructions.size() == 2) {
    const MachineInstr &Move = *Instructions.front();
    unsigned Opcode = Move.getOpcode();
    if (Opcode != AVM::MOV && Opcode != AVM::MOV_RR)
      return false;
    if (Move.getNumExplicitOperands() != 2 || !Move.getOperand(0).isReg() ||
        !Move.getOperand(1).isReg() ||
        !Move.getOperand(0).getReg().isPhysical() ||
        !Move.getOperand(1).getReg().isPhysical())
      return false;
  } else if (Instructions.size() != 1) {
    return false;
  }

  TailSize = 0;
  for (const MachineInstr *MI : Instructions)
    TailSize += MI->getDesc().getSize();
  return TailSize <= TII.get(AVM::JMP8).getSize();
}

static bool hasPhysicalFallthroughInto(const MachineBasicBlock &Target) {
  const MachineFunction &MF = *Target.getParent();
  for (auto I = Target.getIterator(); I != MF.begin();) {
    --I;
    if (!emitsCode(*I))
      continue;
    for (const MachineInstr &MI : llvm::reverse(*I)) {
      if (MI.isDebugInstr() || MI.isMetaInstruction())
        continue;
      return !MI.isBarrier();
    }
  }
  return false;
}

class AVMFinalControlFlow final : public MachineFunctionPass {
public:
  static char ID;
  AVMFinalControlFlow() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return PASS_NAME; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    const AVMInstrInfo &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
    bool Changed = false;

    for (MachineBasicBlock &MBB : MF) {
      SmallVector<MachineInstr *, 2> Last = getLastCodeInstructions(MBB);
      if (Last.size() != 2 || Last.back()->getOpcode() != AVM::RELAX_JMP)
        continue;

      MachineInstr &Cond = *Last.front();
      MachineInstr &Jump = *Last.back();
      unsigned InverseOpcode = getInverseBranchOpcode(Cond.getOpcode());
      if (!InverseOpcode || Cond.getNumExplicitOperands() != 1 ||
          Jump.getNumExplicitOperands() != 1 || !Cond.getOperand(0).isMBB() ||
          !Jump.getOperand(0).isMBB())
        continue;

      MachineBasicBlock *CondTarget = Cond.getOperand(0).getMBB();
      MachineBasicBlock *JumpTarget = Jump.getOperand(0).getMBB();
      MachineBasicBlock *EffectiveSuccessor = getEffectiveLayoutSuccessor(MBB);
      if (CondTarget == JumpTarget)
        continue;

      if (CondTarget != EffectiveSuccessor) {
        // Prefer a single conditional backedge in loops even when another
        // code-emitting block prevents the exit from being the physical
        // fallthrough. The remaining forward jump executes only on exit.
        if (!isLayoutPredecessor(*JumpTarget, MBB))
          continue;
        Cond.setDesc(TII.get(InverseOpcode));
        Cond.getOperand(0).setMBB(JumpTarget);
        Jump.getOperand(0).setMBB(CondTarget);
        Changed = true;
        continue;
      }

      Cond.setDesc(TII.get(InverseOpcode));
      Cond.getOperand(0).setMBB(JumpTarget);
      Jump.eraseFromParent();

      // MachineVerifier models fallthrough through the immediately adjacent
      // block even when that block emits no code. Keep the CFG representation
      // consistent with the physical branch after folding through such a
      // block; the empty block still leads to the same effective successor.
      MachineBasicBlock *ImmediateSuccessor = MBB.getNextNode();
      if (ImmediateSuccessor && ImmediateSuccessor != EffectiveSuccessor &&
          MBB.isSuccessor(EffectiveSuccessor) &&
          !MBB.isSuccessor(ImmediateSuccessor))
        MBB.replaceSuccessor(EffectiveSuccessor, ImmediateSuccessor);
      Changed = true;
    }

    for (MachineBasicBlock &MBB : MF) {
      if (!MBB.succ_empty())
        continue;
      SmallVector<MachineInstr *, 2> Last = getLastCodeInstructions(MBB);
      if (Last.size() != 2 || Last.back()->getOpcode() != AVM::RET)
        continue;

      MachineInstr &Call = *Last.front();
      unsigned JumpOpcode;
      if (Call.getOpcode() == AVM::RELAX_CALL)
        JumpOpcode = AVM::RELAX_JMP;
      else if (Call.getOpcode() == AVM::CALLP)
        JumpOpcode = AVM::JMPP;
      else
        continue;
      if (Call.getNumExplicitOperands() < 1)
        continue;

      BuildMI(MBB, Call, Call.getDebugLoc(), TII.get(JumpOpcode))
          .add(Call.getOperand(0));
      Call.eraseFromParent();
      Last.back()->eraseFromParent();
      Changed = true;
    }

    struct ReturnTail {
      MachineBasicBlock *Predecessor;
      MachineBasicBlock *Target;
    };
    SmallVector<ReturnTail> ReturnTails;
    SmallVector<MachineBasicBlock *> EligibleTargets;

    for (MachineBasicBlock &MBB : MF) {
      SmallVector<MachineInstr *, 2> Last = getLastCodeInstructions(MBB);
      if (Last.empty() || Last.back()->getOpcode() != AVM::RELAX_JMP ||
          Last.back()->getNumExplicitOperands() != 1 ||
          !Last.back()->getOperand(0).isMBB())
        continue;

      MachineBasicBlock *Target = Last.back()->getOperand(0).getMBB();
      if (Target == &MF.front() || Target == &MBB ||
          Target->hasAddressTaken() || Target->isEHPad() ||
          !Target->succ_empty())
        continue;

      unsigned TailSize;
      if (!isEligibleReturnTail(*Target, TII, TailSize))
        continue;

      ReturnTails.push_back({&MBB, Target});
      if (!llvm::is_contained(EligibleTargets, Target))
        EligibleTargets.push_back(Target);
    }

    for (const ReturnTail &Tail : ReturnTails) {
      MachineInstr *Jump = getLastCodeInstructions(*Tail.Predecessor).back();
      for (MachineInstr &MI : *Tail.Target) {
        if (MI.isDebugInstr() || MI.isMetaInstruction())
          continue;
        Tail.Predecessor->insert(Jump->getIterator(),
                                 MF.CloneMachineInstr(&MI));
      }
      Jump->eraseFromParent();
      Tail.Predecessor->removeSuccessor(Tail.Target);
      Changed = true;
    }

    for (MachineBasicBlock *Target : EligibleTargets)
      if (Target->pred_empty() && Target != &MF.front() &&
          !Target->hasAddressTaken() && !hasPhysicalFallthroughInto(*Target))
        Target->eraseFromParent();

    return Changed;
  }
};

} // namespace

char AVMFinalControlFlow::ID = 0;

INITIALIZE_PASS(AVMFinalControlFlow, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMFinalControlFlowPass() {
  return new AVMFinalControlFlow();
}
