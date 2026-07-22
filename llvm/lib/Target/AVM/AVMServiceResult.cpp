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
  case AVM::SYS_MEMCMP_P_PSEUDO:
  case AVM::SYS_STRCMP_P_PSEUDO:
  case AVM::SYS_STRLEN_P_PSEUDO:
  case AVM::SYS_STRNCPY_P_PSEUDO:
  case AVM::SYS_STRNCAT_P_PSEUDO:
  case AVM::SYS_MEMCMP_PSEUDO:
  case AVM::SYS_STRCMP_PSEUDO:
  case AVM::SYS_STRLEN_PSEUDO:
  case AVM::SYS_STRNCPY_PSEUDO:
  case AVM::SYS_STRNCAT_PSEUDO:
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
  case AVM::SYS_STRNCPY_P_PSEUDO:
  case AVM::SYS_STRNCAT_P_PSEUDO:
  case AVM::SYS_STRNCPY_PSEUDO:
  case AVM::SYS_STRNCAT_PSEUDO:
    return true;
  default:
    return false;
  }
}

static bool isFixedServiceRegisterClass(const TargetRegisterClass *RC) {
  return RC == &AVM::R4OnlyRegClass || RC == &AVM::R5OnlyRegClass ||
         RC == &AVM::R6OnlyRegClass || RC == &AVM::Q0OnlyRegClass ||
         RC == &AVM::Q1OnlyRegClass || RC == &AVM::Q2OnlyRegClass ||
         RC == &AVM::Q3OnlyRegClass;
}

static const TargetRegisterClass *
getFixedServiceInputClass(unsigned Opcode, unsigned OperandNo) {
  switch (Opcode) {
  case AVM::SYS_DEBUG_PUTC_PSEUDO:
    return OperandNo == 0 ? &AVM::R4OnlyRegClass : nullptr;

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
  case AVM::SYS_MEMCMP_PSEUDO:
  case AVM::SYS_STRNCPY_PSEUDO:
  case AVM::SYS_STRNCAT_PSEUDO:
    if (OperandNo == 1)
      return &AVM::R4OnlyRegClass;
    if (OperandNo == 2)
      return &AVM::R5OnlyRegClass;
    if (OperandNo == 3)
      return &AVM::R6OnlyRegClass;
    return nullptr;

  case AVM::SYS_STRCMP_PSEUDO:
    if (OperandNo == 1)
      return &AVM::R4OnlyRegClass;
    if (OperandNo == 2)
      return &AVM::R5OnlyRegClass;
    return nullptr;

  case AVM::SYS_STRLEN_PSEUDO:
    return OperandNo == 1 ? &AVM::R4OnlyRegClass : nullptr;

  case AVM::SYS_MEMCMP_P_PSEUDO:
  case AVM::SYS_STRNCPY_P_PSEUDO:
  case AVM::SYS_STRNCAT_P_PSEUDO:
    if (OperandNo == 1)
      return &AVM::R4OnlyRegClass;
    if (OperandNo == 2)
      return &AVM::R5OnlyRegClass;
    if (OperandNo == 3)
      return &AVM::Q3OnlyRegClass;
    return nullptr;

  case AVM::SYS_STRCMP_P_PSEUDO:
    if (OperandNo == 1)
      return &AVM::R4OnlyRegClass;
    if (OperandNo == 2)
      return &AVM::Q3OnlyRegClass;
    return nullptr;

  case AVM::SYS_STRLEN_P_PSEUDO:
    return OperandNo == 1 ? &AVM::Q3OnlyRegClass : nullptr;

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

        Register GeneralResult = MRI.createVirtualRegister(GeneralRC);
        MachineInstrBuilder Copy =
            BuildMI(MBB, std::next(MI.getIterator()), MI.getDebugLoc(),
                    TII.get(TargetOpcode::COPY), GeneralResult)
                .addReg(FixedResult, RegState::Kill);
        Copy->setFlag(MachineInstr::NoMerge);

        for (MachineOperand *Use : Uses) {
          MachineInstr *User = Use->getParent();
          if (const TargetRegisterClass *InputRC = getFixedServiceInputClass(
                  User->getOpcode(), Use->getOperandNo())) {
            Register FixedInput = MRI.createVirtualRegister(InputRC);
            MachineInstrBuilder InputCopy =
                BuildMI(*User->getParent(), User->getIterator(),
                        User->getDebugLoc(), TII.get(TargetOpcode::COPY),
                        FixedInput)
                    .addReg(GeneralResult);
            InputCopy->setFlag(MachineInstr::NoMerge);
            Use->setReg(FixedInput);
          } else {
            Use->setReg(GeneralResult);
            // Selection can already have inserted a fixed-class input copy.
            // Rewriting its source to GeneralResult makes it a general/fixed
            // boundary copy, so protect it just like a copy created here.
            if (User->isCopy() && User->getOperand(0).isReg()) {
              Register CopyDest = User->getOperand(0).getReg();
              if (CopyDest.isVirtual() &&
                  isFixedServiceRegisterClass(MRI.getRegClass(CopyDest)))
                User->setFlag(MachineInstr::NoMerge);
            }
          }
          Use->setIsKill(false);
        }
        Changed = true;
      }
    }
    // The generic spiller rematerializes every use once an instruction is
    // marked rematerializable. Restrict that behavior to reusable immediates;
    // one-use values otherwise perturb allocation without eliminating paired
    // stack traffic.
    for (MachineBasicBlock &MBB : MF) {
      for (MachineInstr &MI : MBB) {
        unsigned Opcode = MI.getOpcode();
        if (Opcode != AVM::LDI8_PSEUDO && Opcode != AVM::LDI16_PSEUDO &&
            Opcode != AVM::LDI32_PSEUDO)
          continue;
        Register Def = MI.getOperand(0).getReg();
        if (!Def.isVirtual() || MRI.use_nodbg_empty(Def) ||
            MRI.hasOneNonDBGUse(Def))
          continue;
        MI.setFlag(MachineInstr::NoMerge);
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
