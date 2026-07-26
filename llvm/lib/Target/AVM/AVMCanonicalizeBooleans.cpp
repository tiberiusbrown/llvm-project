//===-- AVMCanonicalizeBooleans.cpp - Canonical boolean cleanup ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// SelectionDAG promotes general i1 values to i16 with undefined upper bits.
// BRCOND therefore canonicalizes such values with AND 1 even when a PHI cycle
// is provably restricted to zero and one.  Prove the common PHI/XOR-one
// recurrence while machine SSA is still available and remove the redundant
// mask before register allocation.
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/InitializePasses.h"

#include <optional>

using namespace llvm;

#define DEBUG_TYPE "avm-canonicalize-booleans"
#define PASS_NAME "AVM canonical boolean cleanup"

namespace {

static Register stripCopies(Register Reg, MachineRegisterInfo &MRI) {
  SmallDenseSet<Register, 8> Seen;
  while (Reg.isVirtual() && Seen.insert(Reg).second) {
    MachineInstr *Def = MRI.getVRegDef(Reg);
    if (!Def || !Def->isCopy() || Def->getNumExplicitOperands() < 2 ||
        !Def->getOperand(1).isReg())
      break;
    Reg = Def->getOperand(1).getReg();
  }
  return Reg;
}

static std::optional<int64_t> getConstant(Register Reg,
                                          MachineRegisterInfo &MRI) {
  Reg = stripCopies(Reg, MRI);
  if (!Reg.isVirtual())
    return std::nullopt;

  MachineInstr *Def = MRI.getVRegDef(Reg);
  if (!Def || Def->getNumExplicitOperands() < 2 ||
      !Def->getOperand(1).isImm())
    return std::nullopt;

  switch (Def->getOpcode()) {
  case AVM::LDI8_PSEUDO:
  case AVM::LDI16_PSEUDO:
    return Def->getOperand(1).getImm();
  default:
    return std::nullopt;
  }
}

static bool proveBooleanExpression(Register Reg, Register Root,
                                   MachineRegisterInfo &MRI,
                                   SmallDenseSet<Register, 8> &Visiting,
                                   bool &HasSeed) {
  Reg = stripCopies(Reg, MRI);

  // A backedge may refer to the PHI currently being proved.  The PHI is valid
  // only when some other incoming path supplies a concrete canonical seed.
  if (Reg == Root)
    return true;

  if (std::optional<int64_t> C = getConstant(Reg, MRI)) {
    if (*C != 0 && *C != 1)
      return false;
    HasSeed = true;
    return true;
  }

  if (!Reg.isVirtual() || !Visiting.insert(Reg).second)
    return false;

  MachineInstr *Def = MRI.getVRegDef(Reg);
  bool Proven = false;

  if (Def && Def->getOpcode() == AVM::CSET_PSEUDO) {
    HasSeed = true;
    Proven = true;
  } else if (Def && Def->getOpcode() == AVM::XOR16_PSEUDO &&
             Def->getNumExplicitOperands() >= 3 &&
             Def->getOperand(1).isReg() && Def->getOperand(2).isReg()) {
    Register LHS = Def->getOperand(1).getReg();
    Register RHS = Def->getOperand(2).getReg();
    if (std::optional<int64_t> C = getConstant(LHS, MRI); C && *C == 1)
      Proven = proveBooleanExpression(RHS, Root, MRI, Visiting, HasSeed);
    else if (std::optional<int64_t> C = getConstant(RHS, MRI); C && *C == 1)
      Proven = proveBooleanExpression(LHS, Root, MRI, Visiting, HasSeed);
  } else if (Def && Def->getOpcode() == TargetOpcode::PHI &&
             Def->getNumExplicitOperands() >= 3 &&
             (Def->getNumExplicitOperands() & 1) != 0) {
    Proven = true;
    for (unsigned I = 1; I < Def->getNumExplicitOperands(); I += 2) {
      if (!Def->getOperand(I).isReg() ||
          !proveBooleanExpression(Def->getOperand(I).getReg(), Root, MRI,
                                  Visiting, HasSeed)) {
        Proven = false;
        break;
      }
    }
  }

  Visiting.erase(Reg);
  return Proven;
}

static bool isCanonicalBooleanPhi(Register Reg, MachineRegisterInfo &MRI) {
  Reg = stripCopies(Reg, MRI);
  if (!Reg.isVirtual())
    return false;

  MachineInstr *Def = MRI.getVRegDef(Reg);
  if (!Def || Def->getOpcode() != TargetOpcode::PHI ||
      Def->getNumExplicitOperands() < 3 ||
      (Def->getNumExplicitOperands() & 1) == 0)
    return false;

  SmallDenseSet<Register, 8> Visiting;
  Visiting.insert(Reg);
  bool HasSeed = false;
  for (unsigned I = 1; I < Def->getNumExplicitOperands(); I += 2) {
    if (!Def->getOperand(I).isReg() ||
        !proveBooleanExpression(Def->getOperand(I).getReg(), Reg, MRI,
                                Visiting, HasSeed))
      return false;
  }
  return HasSeed;
}

static void eraseDeadConstant(Register Reg, MachineRegisterInfo &MRI) {
  if (!Reg.isVirtual() || !MRI.use_nodbg_empty(Reg))
    return;
  MachineInstr *Def = MRI.getVRegDef(Reg);
  if (!Def)
    return;
  if (Def->getOpcode() == AVM::LDI8_PSEUDO ||
      Def->getOpcode() == AVM::LDI16_PSEUDO)
    Def->eraseFromParent();
}

class AVMCanonicalizeBooleans final : public MachineFunctionPass {
public:
  static char ID;
  AVMCanonicalizeBooleans() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return PASS_NAME; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    MachineRegisterInfo &MRI = MF.getRegInfo();
    assert(MRI.isSSA() && "AVM boolean cleanup requires machine SSA");

    bool Changed = false;
    for (MachineBasicBlock &MBB : MF) {
      for (auto It = MBB.begin(); It != MBB.end();) {
        MachineInstr &MI = *It++;
        if (MI.getOpcode() != AVM::AND16_PSEUDO ||
            MI.getNumExplicitOperands() < 3 ||
            !MI.getOperand(0).isReg() || !MI.getOperand(1).isReg() ||
            !MI.getOperand(2).isReg())
          continue;

        Register Dst = MI.getOperand(0).getReg();
        Register LHS = MI.getOperand(1).getReg();
        Register RHS = MI.getOperand(2).getReg();
        Register BooleanReg;
        Register MaskReg;

        if (std::optional<int64_t> C = getConstant(RHS, MRI); C && *C == 1) {
          BooleanReg = LHS;
          MaskReg = RHS;
        } else if (std::optional<int64_t> C = getConstant(LHS, MRI);
                   C && *C == 1) {
          BooleanReg = RHS;
          MaskReg = LHS;
        } else {
          continue;
        }

        BooleanReg = stripCopies(BooleanReg, MRI);
        if (!isCanonicalBooleanPhi(BooleanReg, MRI))
          continue;

        MRI.replaceRegWith(Dst, BooleanReg);
        MRI.clearKillFlags(BooleanReg);
        MI.eraseFromParent();
        eraseDeadConstant(MaskReg, MRI);
        Changed = true;
      }
    }

    if (Changed)
      MF.verify(this, "After AVM canonical boolean cleanup");
    return Changed;
  }
};

} // namespace

char AVMCanonicalizeBooleans::ID = 0;

INITIALIZE_PASS(AVMCanonicalizeBooleans, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMCanonicalizeBooleansPass() {
  return new AVMCanonicalizeBooleans();
}
