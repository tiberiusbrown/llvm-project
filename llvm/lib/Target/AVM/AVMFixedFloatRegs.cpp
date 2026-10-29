//===-- AVMFixedFloatRegs.cpp - Local fixed FF operands --------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
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
  bool runOnMachineFunction(MachineFunction &MF) override {
    const auto &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
    MachineRegisterInfo &MRI = MF.getRegInfo();
    bool Changed = false;
    SmallSetVector<Register, 16> Relax;
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
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(TargetOpcode::COPY), Fixed)
              .addReg(MO.getReg(), 0, MO.getSubReg());
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
    return Changed;
  }
};
}
char AVMFixedFloatRegs::ID;
INITIALIZE_PASS(AVMFixedFloatRegs, DEBUG_TYPE, "AVM fixed floating-point operands", false, false)
FunctionPass *llvm::createAVMFixedFloatRegsPass() { return new AVMFixedFloatRegs(); }
