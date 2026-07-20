//===-- AVMServiceResult.cpp - Preserve tied service results -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "avm-service-result"
#define PASS_NAME "AVM service result propagation"

namespace {
class AVMServiceResult final : public MachineFunctionPass {
public:
  static char ID;
  AVMServiceResult() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return PASS_NAME; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    MachineRegisterInfo &MRI = MF.getRegInfo();
    bool Changed = false;

    for (MachineBasicBlock &MBB : MF) {
      for (MachineInstr &MI : MBB) {
        switch (MI.getOpcode()) {
        case AVM::SYS_MEMCPY_PSEUDO:
        case AVM::SYS_MEMCPY_P_PSEUDO:
        case AVM::SYS_MEMSET_PSEUDO:
        case AVM::SYS_MEMMOVE_PSEUDO:
          break;
        default:
          continue;
        }

        Register Result = MI.getOperand(0).getReg();
        Register Preserved = MI.getOperand(1).getReg();
        if (!Result.isVirtual() || !Preserved.isVirtual())
          continue;

        // SelectionDAG inserts a fixed-class copy for the tied service input.
        // Follow copy-only setup back to the semantic destination value.
        while (MachineInstr *Def = MRI.getVRegDef(Preserved)) {
          if (!Def->isCopy() || !Def->getOperand(1).isReg())
            break;
          Register Source = Def->getOperand(1).getReg();
          if (!Source.isVirtual())
            break;
          Preserved = Source;
        }

        // The service preserves the complete destination bit pattern. Reuse
        // its tied result for later uses in this block so the old SSA value
        // does not have to be saved across the fixed r4 definition.
        auto It = MI.getIterator();
        ++It;
        for (auto End = MBB.end(); It != End; ++It) {
          for (MachineOperand &MO : It->operands()) {
            if (!MO.isReg() || !MO.isUse() || MO.getReg() != Preserved)
              continue;
            MO.setReg(Result);
            MO.setIsKill(false);
            Changed = true;
          }
        }
      }
    }
    return Changed;
  }
};
} // namespace

char AVMServiceResult::ID = 0;

INITIALIZE_PASS(AVMServiceResult, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMServiceResultPass() {
  return new AVMServiceResult();
}
