//===-- AVMServiceResult.cpp - Preserve tied service results -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
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

static bool isSpriteService(unsigned Opcode) {
  switch (Opcode) {
  case AVM::SYS_DRAW_SPRITE_OVERWRITE_PSEUDO:
  case AVM::SYS_DRAW_SPRITE_PLUS_MASK_PSEUDO:
  case AVM::SYS_DRAW_SPRITE_SELF_MASKED_PSEUDO:
  case AVM::SYS_DRAW_SPRITE_ERASE_PSEUDO:
    return true;
  default:
    return false;
  }
}

struct SpriteCoordinate {
  Register Reg;
  Register Base;
  int64_t Offset = 0;
  SmallVector<MachineInstr *, 2> Setup;
};

static std::optional<SpriteCoordinate>
getSpriteCoordinate(Register Reg, MachineRegisterInfo &MRI) {
  if (!Reg.isVirtual())
    return std::nullopt;

  SpriteCoordinate Result{Reg, Reg};
  SmallVector<MachineInstr *, 2> Defs;
  for (MachineInstr &Def : MRI.def_instructions(Reg))
    Defs.push_back(&Def);

  // An arbitrary definition can serve as the common zero-offset base.
  if (Defs.size() == 1 && !Defs.front()->isCopy() &&
      Defs.front()->getOpcode() != AVM::ADDIS8_PSEUDO)
    return Result;

  // Before two-address conversion the selected add still names its semantic
  // base directly.
  if (Defs.size() == 1 &&
      Defs.front()->getOpcode() == AVM::ADDIS8_PSEUDO) {
    MachineInstr *Add = Defs.front();
    if (!Add->getOperand(1).isReg() || !Add->getOperand(2).isImm())
      return std::nullopt;
    Register Base = Add->getOperand(1).getReg();
    if (!Base.isVirtual() || Base == Reg)
      return std::nullopt;
    Result.Base = Base;
    Result.Offset = Add->getOperand(2).getImm();
    Result.Setup.push_back(Add);
    return Result;
  }

  MachineInstr *Copy = nullptr;
  MachineInstr *Add = nullptr;
  for (MachineInstr *Def : Defs) {
    if (Def->isCopy())
      Copy = Def;
    else if (Def->getOpcode() == AVM::ADDIS8_PSEUDO)
      Add = Def;
    else
      return std::nullopt;
  }
  if (!Copy || !Copy->getOperand(1).isReg())
    return std::nullopt;

  Register Base = Copy->getOperand(1).getReg();
  if (!Base.isVirtual() || Base == Reg)
    return std::nullopt;
  Result.Base = Base;
  Result.Setup.push_back(Copy);

  if (Add) {
    if (Add->getOperand(1).getReg() != Reg ||
        !Add->getOperand(2).isImm())
      return std::nullopt;
    Result.Offset = Add->getOperand(2).getImm();
    Result.Setup.push_back(Add);
  }
  return Result;
}

// Fully unrolled sprite rows commonly leave every base-plus-constant X value
// live across the surrounding row loop. Re-form that arithmetic as one
// loop-local running coordinate. Besides avoiding artificial pressure, this
// lets allocation keep X in r4 across the register-preserving services.
static bool chainSpriteCoordinates(MachineBasicBlock &MBB,
                                   MachineRegisterInfo &MRI,
                                   const TargetInstrInfo &TII,
                                   MachineLoopInfo &MLI) {
  bool Changed = false;

  for (auto It = MBB.begin(); It != MBB.end();) {
    if (!isSpriteService(It->getOpcode())) {
      ++It;
      continue;
    }

    SmallVector<MachineInstr *, 16> Run;
    while (It != MBB.end() && isSpriteService(It->getOpcode())) {
      Run.push_back(&*It);
      ++It;
    }
    if (Run.size() < 2)
      continue;

    auto HintCommonOperand = [&](unsigned OperandNo, Register PhysReg) {
      Register Reg = Run.front()->getOperand(OperandNo).getReg();
      if (!Reg.isVirtual() ||
          !llvm::all_of(Run, [=](const MachineInstr *Sprite) {
            return Sprite->getOperand(OperandNo).getReg() == Reg;
          }))
        return;
      MRI.setRegAllocationHint(Reg, AVMRI::SpriteRun, PhysReg);
    };
    HintCommonOperand(1, AVM::R5);
    HintCommonOperand(2, AVM::R6R7);
    HintCommonOperand(3, AVM::R0);

    Register Pointer = Run.front()->getOperand(2).getReg();
    bool CommonPointer =
        llvm::all_of(Run, [Pointer](const MachineInstr *Sprite) {
          return Sprite->getOperand(2).getReg() == Pointer;
        });
    if (CommonPointer && Pointer.isVirtual()) {
      MachineInstr *PointerDef = MRI.getVRegDef(Pointer);
      MachineLoop *Loop = MLI.getLoopFor(&MBB);
      MachineBasicBlock *Preheader = Loop ? Loop->getLoopPreheader() : nullptr;
      if (PointerDef && PointerDef->getParent() == &MBB && Preheader &&
          PointerDef->getOpcode() == AVM::PROG_ADDR_PSEUDO) {
        PointerDef->removeFromParent();
        Preheader->insert(Preheader->getFirstTerminator(), PointerDef);
        Changed = true;
      }
    }

    SmallVector<SpriteCoordinate, 16> Coordinates;
    bool Valid = true;
    for (MachineInstr *Sprite : Run) {
      auto Coordinate =
          getSpriteCoordinate(Sprite->getOperand(0).getReg(), MRI);
      if (!Coordinate) {
        Valid = false;
        break;
      }
      Coordinates.push_back(std::move(*Coordinate));
    }
    if (!Valid)
      continue;

    Register Base = Coordinates.front().Base;
    int64_t PreviousOffset = Coordinates.front().Offset;
    for (unsigned I = 1; I != Coordinates.size(); ++I) {
      int64_t Delta = Coordinates[I].Offset - PreviousOffset;
      if (Coordinates[I].Base != Base || !isInt<8>(Delta)) {
        Valid = false;
        break;
      }
      for (MachineOperand &Use :
           MRI.use_nodbg_operands(Coordinates[I].Reg)) {
        MachineInstr *User = Use.getParent();
        if (!is_contained(Run, User) || Use.getOperandNo() != 0) {
          Valid = false;
          break;
        }
      }
      if (!Valid)
        break;
      PreviousOffset = Coordinates[I].Offset;
    }
    if (!Valid)
      continue;

    Register Chain = MRI.createVirtualRegister(&AVM::GPR16RegClass);
    MachineInstrBuilder ChainCopy =
        BuildMI(MBB, Run.front()->getIterator(), Run.front()->getDebugLoc(),
                TII.get(TargetOpcode::COPY), Chain)
            .addReg(Coordinates.front().Reg);
    // Keep the loop-invariant base separate from the service-carried
    // coordinate so the latter can remain in r4 across the whole row.
    ChainCopy->setFlag(MachineInstr::NoMerge);
    Run.front()->getOperand(0).setReg(Chain);
    Run.front()->getOperand(0).setIsKill(false);

    PreviousOffset = Coordinates.front().Offset;
    for (unsigned I = 1; I != Coordinates.size(); ++I) {
      int64_t Delta = Coordinates[I].Offset - PreviousOffset;
      if (Delta != 0) {
        Register Next = MRI.createVirtualRegister(&AVM::GPR16RegClass);
        BuildMI(MBB, Run[I]->getIterator(), Run[I]->getDebugLoc(),
                TII.get(AVM::ADDIS8_PSEUDO), Next)
            .addReg(Chain)
            .addImm(Delta);
        Chain = Next;
      }
      Run[I]->getOperand(0).setReg(Chain);
      Run[I]->getOperand(0).setIsKill(false);
      PreviousOffset = Coordinates[I].Offset;
    }

    SmallPtrSet<MachineInstr *, 32> DeadSetup;
    for (unsigned I = 1; I != Coordinates.size(); ++I)
      DeadSetup.insert_range(Coordinates[I].Setup);
    for (MachineInstr *Setup : DeadSetup) {
      Register Def = Setup->getOperand(0).getReg();
      if (Def != Coordinates.front().Reg && MRI.use_nodbg_empty(Def))
        Setup->eraseFromParent();
    }
    Changed = true;
  }

  return Changed;
}

static bool isFixedServiceRegisterClass(const TargetRegisterClass *RC) {
  return RC == &AVM::R0OnlyRegClass || RC == &AVM::R4OnlyRegClass ||
         RC == &AVM::R5OnlyRegClass ||
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

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineLoopInfoWrapperPass>();
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    MachineRegisterInfo &MRI = MF.getRegInfo();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    MachineLoopInfo &MLI = getAnalysis<MachineLoopInfoWrapperPass>().getLI();
    bool Changed = false;

    for (MachineBasicBlock &MBB : MF) {
      Changed |= chainSpriteCoordinates(MBB, MRI, TII, MLI);
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
