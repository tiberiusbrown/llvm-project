//===-- AVMServiceResult.cpp - Preserve tied service results -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "avm-service-result"
#define PASS_NAME "AVM service result propagation"

static const TargetRegisterClass *
getGeneralServiceResultClass(unsigned Opcode) {
  switch (Opcode) {
  case AVM::SYS_MILLIS_PSEUDO:
  case AVM::SYS_MEMCPY_PSEUDO:
  case AVM::SYS_MEMCPY_P_PSEUDO:
  case AVM::SYS_MEMSET_PSEUDO:
  case AVM::SYS_MEMMOVE_PSEUDO:
    return &AVM::GPR16RegClass;

  case AVM::SYS_MILLIS32_PSEUDO:
  case AVM::SYS_SINF_PSEUDO:
  case AVM::SYS_COSF_PSEUDO:
  case AVM::SYS_ATAN2F_PSEUDO:
  case AVM::SYS_TANF_PSEUDO:
  case AVM::SYS_EXPF_PSEUDO:
  case AVM::SYS_LOGF_PSEUDO:
  case AVM::SYS_LOG2F_PSEUDO:
  case AVM::SYS_LOG10F_PSEUDO:
  case AVM::SYS_POWF_PSEUDO:
  case AVM::SYS_HYPOTF_PSEUDO:
  case AVM::SYS_FMODF_PSEUDO:
    return &AVM::GPR32RegClass;

  default:
    return nullptr;
  }
}

static bool isMemoryService(unsigned Opcode) {
  switch (Opcode) {
  case AVM::SYS_MEMCPY_PSEUDO:
  case AVM::SYS_MEMCPY_P_PSEUDO:
  case AVM::SYS_MEMSET_PSEUDO:
  case AVM::SYS_MEMMOVE_PSEUDO:
    return true;
  default:
    return false;
  }
}

static const TargetRegisterClass *getServiceInputClass(unsigned Opcode,
                                                       unsigned OperandNo) {
  switch (Opcode) {
  case AVM::SYS_SINF_PSEUDO:
  case AVM::SYS_COSF_PSEUDO:
  case AVM::SYS_TANF_PSEUDO:
  case AVM::SYS_EXPF_PSEUDO:
  case AVM::SYS_LOGF_PSEUDO:
  case AVM::SYS_LOG2F_PSEUDO:
  case AVM::SYS_LOG10F_PSEUDO:
    return OperandNo == 1 ? &AVM::Q2OnlyRegClass : nullptr;

  case AVM::SYS_ATAN2F_PSEUDO:
  case AVM::SYS_POWF_PSEUDO:
  case AVM::SYS_HYPOTF_PSEUDO:
  case AVM::SYS_FMODF_PSEUDO:
    if (OperandNo == 1)
      return &AVM::Q2OnlyRegClass;
    if (OperandNo == 2)
      return &AVM::Q3OnlyRegClass;
    return nullptr;

  case AVM::SYS_MEMCPY_PSEUDO:
    if (OperandNo == 1)
      return &AVM::R4OnlyRegClass;
    if (OperandNo == 2)
      return &AVM::R5OnlyRegClass;
    if (OperandNo == 3)
      return &AVM::R6OnlyRegClass;
    return nullptr;

  case AVM::SYS_MEMCPY_P_PSEUDO:
    if (OperandNo == 1)
      return &AVM::R4OnlyRegClass;
    if (OperandNo == 2)
      return &AVM::R5OnlyRegClass;
    if (OperandNo == 3)
      return &AVM::Q3OnlyRegClass;
    return nullptr;

  case AVM::SYS_MEMSET_PSEUDO:
    if (OperandNo == 1)
      return &AVM::R4OnlyRegClass;
    if (OperandNo == 2)
      return &AVM::R5OnlyRegClass;
    if (OperandNo == 3)
      return &AVM::R6OnlyRegClass;
    return nullptr;

  case AVM::SYS_MEMMOVE_PSEUDO:
    if (OperandNo == 1)
      return &AVM::R4OnlyRegClass;
    if (OperandNo == 2)
      return &AVM::R5OnlyRegClass;
    if (OperandNo == 3)
      return &AVM::R6OnlyRegClass;
    return nullptr;

  default:
    return nullptr;
  }
}

namespace {
class AVMServiceResult final : public MachineFunctionPass {
public:
  static char ID;
  AVMServiceResult() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return PASS_NAME; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    MachineRegisterInfo &MRI = MF.getRegInfo();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    bool Changed = false;

    for (MachineBasicBlock &MBB : MF) {
      for (MachineInstr &MI : make_early_inc_range(MBB)) {
        const TargetRegisterClass *GeneralRC =
            getGeneralServiceResultClass(MI.getOpcode());
        if (!GeneralRC || MI.getNumOperands() == 0 || !MI.getOperand(0).isReg())
          continue;

        Register FixedResult = MI.getOperand(0).getReg();
        if (!FixedResult.isVirtual())
          continue;

        if (isMemoryService(MI.getOpcode()) && MI.getNumOperands() > 1 &&
            MI.getOperand(1).isReg()) {
          Register Preserved = MI.getOperand(1).getReg();
          if (Preserved.isVirtual()) {
            // SelectionDAG inserts a fixed-class copy for the tied service
            // input. Follow copy-only setup back to the semantic destination
            // value.
            while (MachineInstr *Def = MRI.getVRegDef(Preserved)) {
              if (!Def->isCopy() || !Def->getOperand(1).isReg())
                break;
              Register Source = Def->getOperand(1).getReg();
              if (!Source.isVirtual())
                break;
              Preserved = Source;
            }

            // The service preserves the complete destination bit pattern.
            // Replace only uses after the service, including uses in
            // successor blocks and PHIs.
            SmallVector<MachineOperand *, 8> PreservedUses;
            for (MachineOperand &Use : MRI.use_operands(Preserved)) {
              MachineInstr *User = Use.getParent();
              if (User == &MI)
                continue;

              if (User->getParent() == &MBB) {
                bool IsAfter = false;
                for (auto It = std::next(MI.getIterator()); It != MBB.end();
                     ++It) {
                  if (&*It == User) {
                    IsAfter = true;
                    break;
                  }
                }
                if (!IsAfter)
                  continue;
              }
              PreservedUses.push_back(&Use);
            }

            for (MachineOperand *Use : PreservedUses) {
              Use->setReg(FixedResult);
              Use->setIsKill(false);
              Changed = true;
            }
          }
        }

        SmallVector<MachineOperand *, 8> Uses;
        for (MachineOperand &Use : MRI.use_operands(FixedResult)) {
          MachineInstr *User = Use.getParent();
          if (User == &MI)
            continue;
          Uses.push_back(&Use);
        }
        if (Uses.empty())
          continue;

        bool CrossesService = false;
        for (MachineOperand *Use : Uses) {
          MachineInstr *User = Use->getParent();
          if (User->getParent() != &MBB) {
            CrossesService = true;
            break;
          }
          for (auto It = std::next(MI.getIterator()); It != MBB.end(); ++It) {
            if (&*It == User)
              break;
            if (getGeneralServiceResultClass(It->getOpcode())) {
              CrossesService = true;
              break;
            }
          }
          if (CrossesService)
            break;
        }

        Register GeneralResult = MRI.createVirtualRegister(GeneralRC);
        MachineInstrBuilder Copy =
            BuildMI(MBB, std::next(MI.getIterator()), MI.getDebugLoc(),
                    TII.get(TargetOpcode::COPY), GeneralResult)
                .addReg(FixedResult, RegState::Kill);
        if (CrossesService)
          Copy->setFlag(MachineInstr::NoMerge);

        for (MachineOperand *Use : Uses) {
          MachineInstr *User = Use->getParent();
          if (const TargetRegisterClass *InputRC = getServiceInputClass(
                  User->getOpcode(), Use->getOperandNo())) {
            Register FixedInput = MRI.createVirtualRegister(InputRC);
            BuildMI(*User->getParent(), User->getIterator(),
                    User->getDebugLoc(), TII.get(TargetOpcode::COPY),
                    FixedInput)
                .addReg(GeneralResult);
            Use->setReg(FixedInput);
          } else {
            Use->setReg(GeneralResult);
          }
          Use->setIsKill(false);
        }
        Changed = true;
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
