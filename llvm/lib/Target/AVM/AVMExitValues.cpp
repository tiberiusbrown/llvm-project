//===-- AVMExitValues.cpp - Materialize constants on return edges -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineDomTreeUpdater.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define DEBUG_TYPE "avm-exit-values"

namespace {
class AVMExitValues : public MachineFunctionPass {
public:
  static char ID;
  AVMExitValues() : MachineFunctionPass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineLoopInfoWrapperPass>();
    AU.addRequired<MachineDominatorTreeWrapperPass>();
    AU.addPreserved<MachineLoopInfoWrapperPass>();
    AU.addPreserved<MachineDominatorTreeWrapperPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().set(
        MachineFunctionProperties::Property::IsSSA);
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    MachineRegisterInfo &MRI = MF.getRegInfo();
    MachineLoopInfo &MLI = getAnalysis<MachineLoopInfoWrapperPass>().getLI();
    MachineDominatorTree &MDT =
        getAnalysis<MachineDominatorTreeWrapperPass>().getDomTree();
    MachineDomTreeUpdater MDTU(&MDT, nullptr,
                               MachineDomTreeUpdater::UpdateStrategy::Eager);
    SmallVector<MachineInstr *, 8> Candidates;
    for (MachineBasicBlock &MBB : MF) {
      if (MLI.getLoopFor(&MBB))
        continue;
      for (MachineInstr &MI : MBB)
        if ((MI.getOpcode() == AVM::LDI8_PSEUDO ||
             MI.getOpcode() == AVM::LDI16_PSEUDO) &&
            isUInt<8>(MI.getOperand(1).getImm()))
          Candidates.push_back(&MI);
    }

    bool Changed = false;
    for (MachineInstr *MI : Candidates) {
      Register Reg = MI->getOperand(0).getReg();
      if (!Reg.isVirtual() || !MRI.hasOneDef(Reg) || MRI.use_empty(Reg))
        continue;
      SmallVector<MachineOperand *, 4> Uses;
      bool Safe = true;
      bool HasLoopExit = false;
      for (MachineOperand &Use : MRI.use_operands(Reg)) {
        MachineInstr *Phi = Use.getParent();
        MachineBasicBlock *Exit = Phi->getParent();
        if (!Phi->isPHI() || Use.getSubReg() || !Exit->isReturnBlock() ||
            MLI.getLoopFor(Exit)) {
          Safe = false;
          break;
        }
        MachineBasicBlock *Pred =
            Phi->getOperand(Use.getOperandNo() + 1).getMBB();
        HasLoopExit |= MLI.getLoopFor(Pred) != nullptr;
        if (Pred->succ_size() > 1 && !Pred->canSplitCriticalEdge(Exit, &MLI)) {
          Safe = false;
          break;
        }
        Uses.push_back(&Use);
      }
      if (!Safe || !HasLoopExit)
        continue;

      // A default return value defined before a scan interferes with its
      // pointer for the entire loop. Place each cheap constant on its exit
      // edge instead. In particular, never put an exit-only constant in a
      // header or latch where it would execute on every iteration.
      DenseMap<MachineBasicBlock *, Register> Locals;
      for (MachineOperand *Use : Uses) {
        MachineInstr *Phi = Use->getParent();
        MachineBasicBlock *Exit = Phi->getParent();
        MachineBasicBlock *Pred =
            Phi->getOperand(Use->getOperandNo() + 1).getMBB();
        if (Pred->succ_size() > 1) {
          Pred = Pred->SplitCriticalEdge(Exit, *this, &MDTU);
          assert(Pred && "validated critical edge became unsplittable");
        }
        Register &Local = Locals[Pred];
        if (!Local) {
          Local = MRI.createVirtualRegister(MRI.getRegClass(Reg));
          MachineInstr *Clone = MF.CloneMachineInstr(MI);
          Clone->getOperand(0).setReg(Local);
          Clone->clearFlag(MachineInstr::NoMerge);
          Pred->insert(Pred->getFirstTerminator(), Clone);
        }
        Use->setReg(Local);
      }
      MI->eraseFromParent();
      Changed = true;
    }
    return Changed;
  }
};
} // namespace

char AVMExitValues::ID = 0;
INITIALIZE_PASS_BEGIN(AVMExitValues, DEBUG_TYPE,
                      "AVM return-edge constant materialization", false, false)
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(MachineDominatorTreeWrapperPass)
INITIALIZE_PASS_END(AVMExitValues, DEBUG_TYPE,
                    "AVM return-edge constant materialization", false, false)

FunctionPass *llvm::createAVMExitValuesPass() { return new AVMExitValues(); }
