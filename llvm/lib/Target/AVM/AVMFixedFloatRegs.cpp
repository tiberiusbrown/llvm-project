//===-- AVMFixedFloatRegs.cpp - Local fixed FF operands --------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/InitializePasses.h"

using namespace llvm;
#define DEBUG_TYPE "avm-fixed-float-regs"

namespace {
class AVMFixedFloatRegs : public MachineFunctionPass {
public:
  static char ID;
  AVMFixedFloatRegs() : MachineFunctionPass(ID) {}
  StringRef getPassName() const override { return "AVM fixed floating-point operands"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineLoopInfoWrapperPass>();
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    const auto &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
    MachineRegisterInfo &MRI = MF.getRegInfo();
    MachineLoopInfo &MLI = getAnalysis<MachineLoopInfoWrapperPass>().getLI();
    bool Changed = false;
    SmallSetVector<Register, 16> Relax;
    SmallVector<MachineInstr *, 16> FixedUses;
    SmallPtrSet<const MachineInstr *, 16> CompareUses;
    // SelectionDAG can propagate a singleton through copies and two-address
    // producers which are no longer direct FF operands. Recompute those too.
    // Service-region carriers are introduced by the following pass.
    for (unsigned N = 0; N < MRI.getNumVirtRegs(); ++N) {
      Register Reg = Register::index2VirtReg(N);
      if (MRI.getRegClass(Reg)->getNumRegs() == 1) Relax.insert(Reg);
    }
    for (MachineBasicBlock &MBB : MF) {
      for (auto I = MBB.begin(); I != MBB.end(); ++I) {
        MachineInstr &MI = *I;
        switch (MI.getOpcode()) {
        case AVM::FSQRT_PSEUDO:
        case AVM::FTRUNC_PSEUDO:
        case AVM::FFLOOR_PSEUDO:
        case AVM::FCEIL_PSEUDO:
        case AVM::FROUND_PSEUDO:
        case AVM::FMOD_PSEUDO:
        case AVM::FREMPIO2_PSEUDO:
        case AVM::FMA_PSEUDO:
        case AVM::FADD_PSEUDO: case AVM::FSUB_PSEUDO:
        case AVM::FMUL_PSEUDO: case AVM::FDIV_PSEUDO:
        case AVM::S16TOF_PSEUDO: case AVM::U16TOF_PSEUDO:
        case AVM::FTOS16_PSEUDO: case AVM::FTOU16_PSEUDO:
        case AVM::S32TOF_PSEUDO: case AVM::U32TOF_PSEUDO:
        case AVM::FTOS32_PSEUDO: case AVM::FTOU32_PSEUDO:
        case AVM::FCMP_PSEUDO:
          break;
        default: continue;
        }
        // Keep fixed lifetimes local. A selected singleton result must not
        // force all later uses (or spill temporaries) to occupy that register.
        for (MachineOperand &MO : MI.explicit_operands()) {
          Register Reg = MO.getReg();
          if (!Reg.isVirtual()) continue;
          const TargetRegisterClass *RC = MRI.getRegClass(Reg);
          if (RC->getNumRegs() == 1) Relax.insert(Reg);
          MRI.clearKillFlags(Reg);
        }
        // Singleton classes in the descriptor specify the physical operands.
        // Insert uses first, then the ordinary (not early-clobber) result def.
        // This exposes partial aliases to allocation, including FCMP's r6/q3.
        for (unsigned N = MI.getNumExplicitDefs(); N < MI.getNumExplicitOperands(); ++N) {
          MachineOperand &MO = MI.getOperand(N);
          const TargetRegisterClass *RC = TII.getRegClass(MI.getDesc(), N);
          Register Fixed = RC->getRegister(0);
          MachineInstrBuilder Setup =
              BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(TargetOpcode::COPY), Fixed)
                  .addReg(MO.getReg(), RegState::NoFlags, MO.getSubReg());
          FixedUses.push_back(Setup.getInstr());
          if (MI.getOpcode() == AVM::FCMP_PSEUDO)
            CompareUses.insert(Setup.getInstr());
          MO.setReg(Fixed); MO.setSubReg(0); MO.setIsKill(false);
        }
        for (unsigned N = 0; N < MI.getNumExplicitDefs(); ++N) {
          MachineOperand &Def = MI.getOperand(N);
          Register Result = Def.getReg();
          const TargetRegisterClass *RC = TII.getRegClass(MI.getDesc(), N);
          Register Fixed = RC->getRegister(0);
          Def.setReg(Fixed); Def.setSubReg(0); Def.setIsDead(false);
          BuildMI(MBB, std::next(I), MI.getDebugLoc(), TII.get(TargetOpcode::COPY), Result)
              .addReg(Fixed);
        }
        Changed = true;
      }
    }
    // Recompute all remaining constraints after removing fixed FF uses.
    // Other instructions may still require UpperGPR16, a subregister class,
    // or a fixed service carrier; never broaden past those constraints.
    const auto &TRI = TII.getRegisterInfo();
    for (Register Reg : Relax) {
      const TargetRegisterClass *RC = MRI.getRegClass(Reg);
      RC = AVM::GPR32RegClass.hasSubClassEq(RC)
               ? &AVM::GPR32RegClass : &AVM::GPR16RegClass;
      for (MachineOperand &MO : MRI.reg_nodbg_operands(Reg)) {
        MachineInstr *Use = MO.getParent();
        RC = Use->getRegClassConstraintEffect(MO.getOperandNo(), RC, &TII, &TRI);
        assert(RC && "incompatible remaining FF value constraints");
      }
      MRI.setRegClass(Reg, RC);
    }
    // SelectionDAG may schedule a constant at the top of a block although its
    // only use is a later fixed operand. Keeping a pair live across intervening
    // services can spill the values loaded for those services. Move the pure
    // immediate and its one-use copy carriers next to the fixed setup instead.
    // Staying in the same block preserves the materialization frequency.
    for (MachineInstr *Setup : FixedUses) {
      if (!Setup->isFullCopy())
        continue;
      MachineBasicBlock &MBB = *Setup->getParent();
      // Repeated setup in a loop amortizes shortening the invariant lifetime.
      // In straight-line math kernels, scheduling the immediate earlier can
      // instead preserve a useful copy hint and avoid extra input shuffles.
      if (!MLI.getLoopFor(&MBB))
        continue;
      Register Reg = Setup->getOperand(1).getReg();
      SmallVector<MachineInstr *, 4> Chain;
      MachineInstr *Use = Setup;
      while (Reg.isVirtual()) {
        MachineOperand *OnlyUse = MRI.getOneNonDBGUse(Reg);
        MachineInstr *Def = MRI.getUniqueVRegDef(Reg);
        // Preserve availability at any intervening variable debug locations.
        if (!MRI.hasOneUse(Reg) || !OnlyUse || OnlyUse->getParent() != Use ||
            !Def || Def->getParent() != &MBB || Def->isBundled())
          break;
        Chain.push_back(Def);
        if (Def->getOpcode() == AVM::LDI8_PSEUDO ||
            Def->getOpcode() == AVM::LDI16_PSEUDO ||
            Def->getOpcode() == AVM::LDI32_PSEUDO) {
          for (MachineInstr *Move : llvm::reverse(Chain))
            MBB.splice(Setup->getIterator(), &MBB, Move->getIterator());
          Changed = true;
          break;
        }
        if (!Def->isFullCopy())
          break;
        Reg = Def->getOperand(1).getReg();
        Use = Def;
      }
    }
    for (unsigned I = 0, E = MRI.getNumVirtRegs(); I != E; ++I) {
      Register Reg = Register::index2VirtReg(I);
      if (MRI.getRegClass(Reg) != &AVM::GPR32RegClass)
        continue;
      const MachineInstr *Def = MRI.getUniqueVRegDef(Reg);
      if (!Def || Def->getOpcode() != AVM::LOAD32_PSEUDO ||
          !all_of(MRI.use_nodbg_instructions(Reg),
                  [&](const MachineInstr &Use) {
                    return Use.getParent() == Def->getParent();
                  }) ||
          count_if(MRI.use_nodbg_instructions(Reg),
                   [](const MachineInstr &Use) {
                     return Use.getOpcode() == AVM::CMOV32_PSEUDO;
                   }) < 2)
        continue;
      Register Fixed;
      bool Clobbered = false;
      for (const MachineInstr &MI : *Def->getParent()) {
        bool OwnSetup = MI.isFullCopy() && MI.getOperand(1).getReg() == Reg &&
                        (MI.getOperand(0).getReg() == AVM::R4R5 ||
                         MI.getOperand(0).getReg() == AVM::R6R7);
        if (!Fixed && OwnSetup && CompareUses.contains(&MI))
          Fixed = MI.getOperand(0).getReg();
        if (Fixed && !OwnSetup && MI.modifiesRegister(Fixed, &TRI))
          Clobbered = true;
        if (Clobbered && MI.getOpcode() == AVM::CMOV32_PSEUDO &&
            MI.readsRegister(Reg, &TRI)) {
          // Retaining this value avoids a pair store and repeated reloads at
          // fixed operand setups. Prioritize its contiguous pair before
          // ordinary scalar recurrences fragment the small register file.
          MRI.setRegClass(Reg, &AVM::RetainedFP32RegClass);
          Changed = true;
          break;
        }
      }
    }
    return Changed;
  }
};
}
char AVMFixedFloatRegs::ID;
INITIALIZE_PASS_BEGIN(AVMFixedFloatRegs, DEBUG_TYPE, "AVM fixed floating-point operands", false, false)
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfoWrapperPass)
INITIALIZE_PASS_END(AVMFixedFloatRegs, DEBUG_TYPE, "AVM fixed floating-point operands", false, false)
FunctionPass *llvm::createAVMFixedFloatRegsPass() { return new AVMFixedFloatRegs(); }
