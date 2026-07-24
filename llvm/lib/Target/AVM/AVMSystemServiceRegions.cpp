//===-- AVMSystemServiceRegions.cpp - Fixed service regions --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "AVMSystemServiceInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/MathExtras.h"

#include <optional>

using namespace llvm;

#define DEBUG_TYPE "avm-system-service-regions"
#define PASS_NAME "AVM fixed-register system-service regions"

namespace {

struct ServiceUse {
  MachineInstr *MI;
  const AVMServiceInputInfo *Input;
  unsigned OperandNo;
  unsigned ServiceIndex;
};

struct AffineValue {
  Register Reg;
  Register Base;
  int64_t Offset = 0;
  SmallVector<MachineInstr *, 2> Setup;
};

static bool aliasesRequiredServiceRegister(Register Reg,
                                           ArrayRef<MCPhysReg> RequiredRegs,
                                           const TargetRegisterInfo &TRI) {
  if (!Reg.isPhysical())
    return false;
  for (MCPhysReg Required : RequiredRegs)
    if (TRI.regsOverlap(Reg.asMCReg(), Required))
      return true;
  return false;
}

static bool isRegionBoundary(const MachineInstr &MI,
                             ArrayRef<MCPhysReg> RequiredRegs,
                             const TargetRegisterInfo &TRI) {
  if (isAVMSystemService(MI.getOpcode()))
    return false;
  if (MI.isTerminator() || MI.isCall() || MI.isInlineAsm() || MI.isBarrier() ||
      MI.hasUnmodeledSideEffects() ||
      llvm::any_of(MI.operands(),
                   [](const MachineOperand &MO) { return MO.isRegMask(); }))
    return true;
  return llvm::any_of(MI.operands(), [&](const MachineOperand &MO) {
    return MO.isReg() && MO.isDef() && !MO.isImplicit() &&
           aliasesRequiredServiceRegister(MO.getReg(), RequiredRegs, TRI);
  });
}

static SmallVector<ServiceUse, 8>
collectServiceUses(ArrayRef<MachineInstr *> Services) {
  SmallVector<ServiceUse, 8> Uses;
  for (auto [ServiceIndex, MI] : llvm::enumerate(Services)) {
    const AVMSystemServiceInfo &Info =
        getRequiredAVMSystemServiceInfo(MI->getOpcode());
    unsigned OperandNo = Info.Outputs.size();
    for (const AVMServiceInputInfo &Input : Info.Inputs) {
      if (!Input.PassToMachine)
        continue;
      assert(OperandNo < MI->getNumOperands() &&
             MI->getOperand(OperandNo).isReg() &&
             "malformed semantic service pseudo");
      Uses.push_back(
          {MI, &Input, OperandNo, static_cast<unsigned>(ServiceIndex)});
      ++OperandNo;
    }
  }
  return Uses;
}

static bool serviceOutputAliases(const MachineInstr &MI, MCPhysReg PhysReg,
                                 const TargetRegisterInfo &TRI) {
  const AVMSystemServiceInfo &Info =
      getRequiredAVMSystemServiceInfo(MI.getOpcode());
  return llvm::any_of(Info.Outputs, [&](const AVMServiceOutputInfo &Output) {
    return TRI.regsOverlap(Output.PhysReg, PhysReg);
  });
}

static bool hasOutputBetween(ArrayRef<MachineInstr *> Services,
                             unsigned FirstIndex, unsigned LastIndex,
                             MCPhysReg PhysReg, const TargetRegisterInfo &TRI) {
  for (unsigned I = FirstIndex; I < LastIndex; ++I)
    if (serviceOutputAliases(*Services[I], PhysReg, TRI))
      return true;
  return false;
}

static std::optional<AffineValue> decomposeAffineI16(Register Reg,
                                                     MachineRegisterInfo &MRI) {
  if (!Reg.isVirtual())
    return std::nullopt;

  AffineValue Result{Reg, Reg};
  SmallVector<MachineInstr *, 2> Defs;
  for (MachineInstr &Def : MRI.def_instructions(Reg))
    Defs.push_back(&Def);

  if (Defs.size() == 1 && !Defs.front()->isCopy() &&
      Defs.front()->getOpcode() != AVM::ADDIS8_PSEUDO)
    return Result;

  if (Defs.size() == 1 && Defs.front()->getOpcode() == AVM::ADDIS8_PSEUDO) {
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
    if (!Add->getOperand(1).isReg() || Add->getOperand(1).getReg() != Reg ||
        !Add->getOperand(2).isImm())
      return std::nullopt;
    Result.Offset = Add->getOperand(2).getImm();
    Result.Setup.push_back(Add);
  }
  return Result;
}

static bool setupIsSafe(const AffineValue &Value) {
  return llvm::all_of(Value.Setup, [](const MachineInstr *MI) {
    // Affine decomposition admits only COPY and ADDIS8_PSEUDO.  The latter is
    // conservatively marked as unmodelled by TableGen, but is still a pure
    // register-plus-immediate operation.
    return !MI->mayLoadOrStore() && !MI->isCall() && !MI->isInlineAsm() &&
           !MI->isBarrier();
  });
}

static bool chainAffineInputs(ArrayRef<MachineInstr *> Services,
                              MachineRegisterInfo &MRI,
                              const TargetInstrInfo &TII,
                              const TargetRegisterInfo &TRI) {
  bool Changed = false;
  SmallVector<ServiceUse, 8> AllUses = collectServiceUses(Services);

  for (unsigned Start = 0; Start < AllUses.size(); ++Start) {
    const ServiceUse &Seed = AllUses[Start];
    if (Seed.Input->Kind != AVMServiceValueKind::I16)
      continue;
    bool EarlierEquivalent = llvm::any_of(
        ArrayRef(AllUses).take_front(Start), [&](const ServiceUse &Use) {
          return Use.Input->Kind == AVMServiceValueKind::I16 &&
                 Use.Input->LogicalArgumentIndex ==
                     Seed.Input->LogicalArgumentIndex &&
                 Use.Input->PhysReg == Seed.Input->PhysReg &&
                 !hasOutputBetween(Services, Use.ServiceIndex,
                                   Seed.ServiceIndex, Seed.Input->PhysReg, TRI);
        });
    if (EarlierEquivalent)
      continue;

    SmallVector<ServiceUse, 8> Run;
    unsigned LastServiceIndex = Seed.ServiceIndex;
    for (const ServiceUse &Use : ArrayRef(AllUses).drop_front(Start)) {
      if (Use.Input->Kind != AVMServiceValueKind::I16 ||
          Use.Input->LogicalArgumentIndex != Seed.Input->LogicalArgumentIndex ||
          Use.Input->PhysReg != Seed.Input->PhysReg)
        continue;
      if (!Run.empty() &&
          hasOutputBetween(Services, LastServiceIndex, Use.ServiceIndex,
                           Seed.Input->PhysReg, TRI))
        break;
      Run.push_back(Use);
      LastServiceIndex = Use.ServiceIndex;
    }
    if (Run.size() < 2)
      continue;

    SmallVector<AffineValue, 8> Values;
    bool Valid = true;
    for (const ServiceUse &Use : Run) {
      auto Value =
          decomposeAffineI16(Use.MI->getOperand(Use.OperandNo).getReg(), MRI);
      if (!Value || !setupIsSafe(*Value)) {
        Valid = false;
        break;
      }
      Values.push_back(std::move(*Value));
    }
    if (!Valid || llvm::all_of(Values, [&](const AffineValue &Value) {
          return Value.Reg == Values.front().Reg;
        }))
      continue;

    Register Base = Values.front().Base;
    int64_t PreviousOffset = Values.front().Offset;
    for (unsigned I = 1; I != Values.size(); ++I) {
      int64_t Delta = Values[I].Offset - PreviousOffset;
      if (Values[I].Base != Base || !isInt<8>(Delta)) {
        Valid = false;
        break;
      }
      for (MachineOperand &Use : MRI.use_nodbg_operands(Values[I].Reg)) {
        MachineInstr *User = Use.getParent();
        if ((User != Run[I].MI || Use.getOperandNo() != Run[I].OperandNo) &&
            !llvm::is_contained(Values[I].Setup, User)) {
          Valid = false;
          break;
        }
      }
      if (!Valid)
        break;
      PreviousOffset = Values[I].Offset;
    }
    if (!Valid)
      continue;

    const TargetRegisterClass *FixedRC =
        getAVMFixedRegisterClass(Seed.Input->PhysReg, AVMServiceValueKind::I16);
    assert(FixedRC && "invalid fixed i16 service register");
    MachineBasicBlock &MBB = *Run.front().MI->getParent();
    Register Chain = MRI.createVirtualRegister(FixedRC);
    MachineInstrBuilder Initial = BuildMI(MBB, Run.front().MI->getIterator(),
                                          Run.front().MI->getDebugLoc(),
                                          TII.get(TargetOpcode::COPY), Chain)
                                      .addReg(Values.front().Reg);
    Initial->setFlag(MachineInstr::NoMerge);
    Run.front().MI->getOperand(Run.front().OperandNo).setReg(Chain);
    Run.front().MI->getOperand(Run.front().OperandNo).setIsKill(false);

    PreviousOffset = Values.front().Offset;
    for (unsigned I = 1; I != Values.size(); ++I) {
      int64_t Delta = Values[I].Offset - PreviousOffset;
      if (Delta) {
        Register Next = MRI.createVirtualRegister(FixedRC);
        BuildMI(MBB, Run[I].MI->getIterator(), Run[I].MI->getDebugLoc(),
                TII.get(AVM::ADDIS8_PSEUDO), Next)
            .addReg(Chain)
            .addImm(Delta);
        Chain = Next;
      }
      Run[I].MI->getOperand(Run[I].OperandNo).setReg(Chain);
      Run[I].MI->getOperand(Run[I].OperandNo).setIsKill(false);
      PreviousOffset = Values[I].Offset;
    }

    SmallPtrSet<MachineInstr *, 16> DeadSetup;
    for (unsigned I = 1; I != Values.size(); ++I)
      DeadSetup.insert_range(Values[I].Setup);
    for (MachineInstr *Setup : DeadSetup) {
      Register Def = Setup->getOperand(0).getReg();
      if (Def != Values.front().Reg && MRI.use_nodbg_empty(Def))
        Setup->eraseFromParent();
    }
    Changed = true;
  }
  return Changed;
}

static const AVMServiceInputInfo *getInputForOperand(const MachineInstr &MI,
                                                     unsigned OperandNo) {
  const AVMSystemServiceInfo *Info = getAVMSystemServiceInfo(MI.getOpcode());
  if (!Info)
    return nullptr;
  unsigned Current = Info->Outputs.size();
  for (const AVMServiceInputInfo &Input : Info->Inputs) {
    if (!Input.PassToMachine)
      continue;
    if (Current == OperandNo)
      return &Input;
    ++Current;
  }
  return nullptr;
}

static bool formTiedResultChains(ArrayRef<MachineInstr *> Services,
                                 MachineRegisterInfo &MRI,
                                 const TargetInstrInfo &TII,
                                 const TargetRegisterInfo &TRI) {
  bool Changed = false;
  for (auto [ServiceIndex, MI] : llvm::enumerate(Services)) {
    const AVMSystemServiceInfo &Info =
        getRequiredAVMSystemServiceInfo(MI->getOpcode());
    for (const AVMServiceOutputInfo &Output : Info.Outputs) {
      if (Output.TiedLogicalInput < 0)
        continue;
      assert(Output.ResultIndex < Info.Outputs.size() &&
             "invalid service result index");
      MachineOperand &OutputMO = MI->getOperand(Output.ResultIndex);
      if (!OutputMO.isReg() || !OutputMO.getReg().isVirtual())
        continue;
      Register GeneralResult = OutputMO.getReg();

      SmallVector<MachineOperand *, 4> CoveredUses;
      SmallVector<MachineOperand *, 4> EscapingUses;
      for (MachineOperand &Use : MRI.use_nodbg_operands(GeneralResult)) {
        MachineInstr *User = Use.getParent();
        auto Found = llvm::find(Services, User);
        const AVMServiceInputInfo *LaterInput =
            Found == Services.end()
                ? nullptr
                : getInputForOperand(*User, Use.getOperandNo());
        if (LaterInput && LaterInput->PhysReg == Output.PhysReg &&
            static_cast<unsigned>(Found - Services.begin()) > ServiceIndex &&
            !hasOutputBetween(Services, ServiceIndex + 1,
                              static_cast<unsigned>(Found - Services.begin()),
                              Output.PhysReg, TRI))
          CoveredUses.push_back(&Use);
        else
          EscapingUses.push_back(&Use);
      }
      if (CoveredUses.empty())
        continue;

      int TiedOperandNo = getAVMServiceInputOperandIndex(
          Info, static_cast<unsigned>(Output.TiedLogicalInput));
      assert(TiedOperandNo >= 0 && "tied service input must be explicit");
      Register InputReg = MI->getOperand(TiedOperandNo).getReg();
      const TargetRegisterClass *FixedRC =
          getAVMFixedRegisterClass(Output.PhysReg, Output.Kind);
      assert(FixedRC && "invalid tied service output register");
      Register FixedInput = MRI.createVirtualRegister(FixedRC);
      MachineInstrBuilder InCopy =
          BuildMI(*MI->getParent(), MI->getIterator(), MI->getDebugLoc(),
                  TII.get(TargetOpcode::COPY), FixedInput)
              .addReg(InputReg);
      InCopy->setFlag(MachineInstr::NoMerge);
      Register FixedResult = MRI.createVirtualRegister(FixedRC);
      OutputMO.setReg(FixedResult);
      MI->getOperand(TiedOperandNo).setReg(FixedInput);
      for (MachineOperand *Use : CoveredUses) {
        Use->setReg(FixedResult);
        Use->setIsKill(false);
      }
      if (!EscapingUses.empty()) {
        MachineInstrBuilder OutCopy =
            BuildMI(*MI->getParent(), std::next(MI->getIterator()),
                    MI->getDebugLoc(), TII.get(TargetOpcode::COPY),
                    GeneralResult)
                .addReg(FixedResult);
        OutCopy->setFlag(MachineInstr::NoMerge);
      }
      Changed = true;
    }
  }
  return Changed;
}

struct CarrierGroup {
  MCPhysReg PhysReg;
  AVMServiceValueKind Kind;
  Register Value;
  SmallVector<ServiceUse, 4> Uses;
};

static bool isConstantMaterialization(const MachineInstr &MI) {
  if (isAVMSystemService(MI.getOpcode()))
    return false;

  bool HasConstantSource = false;
  for (const MachineOperand &MO : MI.operands()) {
    if (MO.isImplicit() || (MO.isReg() && MO.isDef()))
      continue;
    if (MO.isImm() || MO.isCImm() || MO.isFPImm() || MO.isGlobal() ||
        MO.isBlockAddress() || MO.isSymbol() || MO.isCPI() || MO.isJTI()) {
      HasConstantSource = true;
      continue;
    }
    return false;
  }
  return HasConstantSource;
}

static bool canHoistDefinition(MachineInstr &Def, MachineLoop &Loop,
                               const TargetInstrInfo &TII) {
  bool ConstantMaterialization = isConstantMaterialization(Def);
  return Loop.contains(&Def) &&
         Loop.isLoopInvariant(Def, Def.getOperand(0).getReg()) &&
         (Def.isCopy() || ConstantMaterialization ||
          TII.isTriviallyReMaterializable(Def)) &&
         !Def.mayLoadOrStore() &&
         (!Def.hasUnmodeledSideEffects() || ConstantMaterialization) &&
         !Def.isCall() && !Def.isInlineAsm() && !Def.isBarrier();
}

static bool createCommonCarriers(ArrayRef<MachineInstr *> Services,
                                 MachineRegisterInfo &MRI, MachineLoopInfo &MLI,
                                 MachineDominatorTree &MDT,
                                 const TargetInstrInfo &TII,
                                 const TargetRegisterInfo &TRI) {
  (void)MDT;
  SmallVector<CarrierGroup, 8> Groups;
  for (const ServiceUse &Use : collectServiceUses(Services)) {
    Register Value = Use.MI->getOperand(Use.OperandNo).getReg();
    if (!Value.isVirtual())
      continue;
    CarrierGroup *Group = nullptr;
    for (CarrierGroup &Candidate : llvm::reverse(Groups)) {
      if (Candidate.PhysReg != Use.Input->PhysReg ||
          Candidate.Kind != Use.Input->Kind || Candidate.Value != Value)
        continue;
      if (!hasOutputBetween(Services, Candidate.Uses.back().ServiceIndex,
                            Use.ServiceIndex, Use.Input->PhysReg, TRI))
        Group = &Candidate;
      break;
    }
    if (!Group) {
      Groups.push_back({Use.Input->PhysReg, Use.Input->Kind, Value, {Use}});
      continue;
    }
    Group->Uses.push_back(Use);
  }

  bool Changed = false;
  for (CarrierGroup &Group : Groups) {
    const TargetRegisterClass *FixedRC =
        getAVMFixedRegisterClass(Group.PhysReg, Group.Kind);
    assert(FixedRC && "invalid fixed service input register");
    if (MRI.getRegClass(Group.Value) == FixedRC)
      continue;

    MachineBasicBlock *UseBlock = Group.Uses.front().MI->getParent();
    MachineLoop *Loop = MLI.getLoopFor(UseBlock);
    bool LoopInvariant = false;
    MachineBasicBlock *Preheader = nullptr;
    MachineInstr *Def = MRI.getVRegDef(Group.Value);
    if (Loop && llvm::all_of(Group.Uses, [&](const ServiceUse &Use) {
          return Loop->contains(Use.MI->getParent());
        })) {
      if (!Def || !Loop->contains(Def))
        LoopInvariant = true;
      else if (canHoistDefinition(*Def, *Loop, TII))
        LoopInvariant = true;
      if (LoopInvariant)
        Preheader = Loop->getLoopPreheader();
      if (!Preheader)
        LoopInvariant = false;
    }
    if (Group.Uses.size() < 2 && !LoopInvariant)
      continue;

    MachineBasicBlock *InsertBlock = UseBlock;
    MachineBasicBlock::iterator InsertAt = Group.Uses.front().MI->getIterator();
    if (LoopInvariant) {
      InsertBlock = Preheader;
      InsertAt = Preheader->getFirstTerminator();
      if (Def && Loop->contains(Def)) {
        Def->removeFromParent();
        Preheader->insert(InsertAt, Def);
      }
    }

    Register Carrier = MRI.createVirtualRegister(FixedRC);
    MachineInstrBuilder Copy =
        BuildMI(*InsertBlock, InsertAt, Group.Uses.front().MI->getDebugLoc(),
                TII.get(TargetOpcode::COPY), Carrier)
            .addReg(Group.Value);
    Copy->setFlag(MachineInstr::NoMerge);
    for (const ServiceUse &Use : Group.Uses) {
      Use.MI->getOperand(Use.OperandNo).setReg(Carrier);
      Use.MI->getOperand(Use.OperandNo).setIsKill(false);
    }
    Changed = true;
  }
  return Changed;
}

class AVMSystemServiceRegions final : public MachineFunctionPass {
public:
  static char ID;
  AVMSystemServiceRegions() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return PASS_NAME; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineLoopInfoWrapperPass>();
    AU.addRequired<MachineDominatorTreeWrapperPass>();
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    MachineRegisterInfo &MRI = MF.getRegInfo();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
    MachineLoopInfo &MLI = getAnalysis<MachineLoopInfoWrapperPass>().getLI();
    MachineDominatorTree &MDT =
        getAnalysis<MachineDominatorTreeWrapperPass>().getDomTree();
    bool Changed = false;
    SmallVector<MCPhysReg, 8> RequiredRegs;
    for (MachineBasicBlock &MBB : MF)
      for (MachineInstr &MI : MBB)
        if (const AVMSystemServiceInfo *Info =
                getAVMSystemServiceInfo(MI.getOpcode())) {
          for (const AVMServiceInputInfo &Input : Info->Inputs)
            if (Input.PassToMachine)
              RequiredRegs.push_back(Input.PhysReg);
          for (const AVMServiceOutputInfo &Output : Info->Outputs)
            RequiredRegs.push_back(Output.PhysReg);
        }
    llvm::sort(RequiredRegs);
    RequiredRegs.erase(std::unique(RequiredRegs.begin(), RequiredRegs.end()),
                       RequiredRegs.end());

    if (MF.getTarget().getOptLevel() != CodeGenOptLevel::None) {
      for (MachineBasicBlock &MBB : MF) {
        auto RegionBegin = MBB.begin();
        while (RegionBegin != MBB.end()) {
          while (RegionBegin != MBB.end() &&
                 isRegionBoundary(*RegionBegin, RequiredRegs, TRI))
            ++RegionBegin;
          if (RegionBegin == MBB.end())
            break;
          auto RegionEnd = RegionBegin;
          SmallVector<MachineInstr *, 8> Services;
          while (RegionEnd != MBB.end() &&
                 !isRegionBoundary(*RegionEnd, RequiredRegs, TRI)) {
            if (isAVMSystemService(RegionEnd->getOpcode()))
              Services.push_back(&*RegionEnd);
            ++RegionEnd;
          }
          if (!Services.empty()) {
            Changed |= formTiedResultChains(Services, MRI, TII, TRI);
            Changed |= chainAffineInputs(Services, MRI, TII, TRI);
            Changed |= createCommonCarriers(Services, MRI, MLI, MDT, TII, TRI);
          }
          RegionBegin = RegionEnd;
        }
      }
    }

    // Avoid aggressive one-use rematerialization perturbing fixed-carrier
    // allocation while retaining profitable multi-use immediates.
    for (MachineBasicBlock &MBB : MF)
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
    if (Changed)
      MF.verify(this, "After AVM fixed-register system-service regions");
    return Changed;
  }
};

} // namespace

char AVMSystemServiceRegions::ID = 0;

INITIALIZE_PASS_BEGIN(AVMSystemServiceRegions, DEBUG_TYPE, PASS_NAME, false,
                      false)
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(MachineDominatorTreeWrapperPass)
INITIALIZE_PASS_END(AVMSystemServiceRegions, DEBUG_TYPE, PASS_NAME, false,
                    false)

FunctionPass *llvm::createAVMSystemServiceRegionsPass() {
  return new AVMSystemServiceRegions();
}
