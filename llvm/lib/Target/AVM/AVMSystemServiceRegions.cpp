//===-- AVMSystemServiceRegions.cpp - Fixed service regions --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMInstrInfo.h"
#include "AVMSystemServiceInfo.h"
#include "llvm/ADT/DenseMap.h"
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
  int Scale = 1;
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

static bool areEquivalentServiceValues(Register LHS, Register RHS,
                                       MachineRegisterInfo &MRI);

static bool serviceHasConflictingInput(const MachineInstr &MI,
                                       MCPhysReg PhysReg, Register Value,
                                       MachineRegisterInfo &MRI,
                                       const TargetRegisterInfo &TRI) {
  const AVMSystemServiceInfo &Info =
      getRequiredAVMSystemServiceInfo(MI.getOpcode());
  unsigned OperandNo = Info.Outputs.size();
  for (const AVMServiceInputInfo &Input : Info.Inputs) {
    if (!Input.PassToMachine)
      continue;
    const MachineOperand &MO = MI.getOperand(OperandNo++);
    if (TRI.regsOverlap(Input.PhysReg, PhysReg) && MO.isReg() &&
        !areEquivalentServiceValues(MO.getReg(), Value, MRI))
      return true;
  }
  return false;
}

static bool hasConflictingInputBetween(ArrayRef<MachineInstr *> Services,
                                       unsigned FirstIndex,
                                       unsigned LastIndex, MCPhysReg PhysReg,
                                       Register Value,
                                       MachineRegisterInfo &MRI,
                                       const TargetRegisterInfo &TRI) {
  for (unsigned I = FirstIndex + 1; I < LastIndex; ++I)
    if (serviceHasConflictingInput(*Services[I], PhysReg, Value, MRI, TRI))
      return true;
  return false;
}

static std::optional<int64_t> getI16Immediate(Register Reg,
                                             MachineRegisterInfo &MRI) {
  if (!Reg.isVirtual())
    return std::nullopt;
  MachineInstr *Def = MRI.getUniqueVRegDef(Reg);
  if (!Def || (Def->getOpcode() != AVM::LDI8_PSEUDO &&
               Def->getOpcode() != AVM::LDI16_PSEUDO) ||
      !Def->getOperand(1).isImm())
    return std::nullopt;
  return Def->getOperand(1).getImm();
}

static std::optional<AffineValue> decomposeAffineI16(Register Reg,
                                                     MachineRegisterInfo &MRI,
                                                     unsigned Depth = 0) {
  if (!Reg.isVirtual())
    return std::nullopt;

  AffineValue Result{Reg, Reg, 0, {}};
  SmallVector<MachineInstr *, 2> Defs;
  for (MachineInstr &Def : MRI.def_instructions(Reg))
    Defs.push_back(&Def);

  // Normalize equivalent SSA arithmetic forms before matching service inputs.
  // In particular, InstCombine rewrites (0 - base) + C into C - base.
  if (Defs.size() == 1 && Depth < 8) {
    MachineInstr *Def = Defs.front();
    Register Source;
    int Scale = 1;
    int64_t Offset = 0;
    if (Def->isCopy()) {
      Source = Def->getOperand(1).getReg();
    } else if (Def->getOpcode() == AVM::ADDIS8_PSEUDO) {
      Source = Def->getOperand(1).getReg();
      Offset = Def->getOperand(2).getImm();
    } else if (Def->getOpcode() == AVM::ADD16_PSEUDO ||
               Def->getOpcode() == AVM::SUB16_PSEUDO) {
      Register LHS = Def->getOperand(1).getReg();
      Register RHS = Def->getOperand(2).getReg();
      if (auto Immediate = getI16Immediate(RHS, MRI)) {
        Source = LHS;
        Offset = Def->getOpcode() == AVM::SUB16_PSEUDO ? -*Immediate : *Immediate;
      } else if (auto Immediate = getI16Immediate(LHS, MRI)) {
        Source = RHS;
        Scale = Def->getOpcode() == AVM::SUB16_PSEUDO ? -1 : 1;
        Offset = *Immediate;
      }
    }
    if (Source.isVirtual() && Source != Reg) {
      auto Base = decomposeAffineI16(Source, MRI, Depth + 1);
      if (!Base)
        return std::nullopt;
      Result.Base = Base->Base;
      Result.Scale = Scale * Base->Scale;
      Result.Offset = Offset + Scale * Base->Offset;
      Result.Setup = std::move(Base->Setup);
      Result.Setup.push_back(Def);
      return Result;
    }
  }

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
    // Only copies and pure affine arithmetic are admitted by decomposition.
    return !MI->mayLoadOrStore() && !MI->isCall() && !MI->isInlineAsm() &&
           !MI->isBarrier();
  });
}

static bool chainAffineInputs(ArrayRef<MachineInstr *> Services,
                              MachineRegisterInfo &MRI,
                              const TargetInstrInfo &TII,
                              const TargetRegisterInfo &TRI,
                              MachineDominatorTree *MDT = nullptr,
                              const DenseMap<MachineInstr *, MachineInstr *> *
                                  SetupPoints = nullptr) {
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

    SmallPtrSet<MachineInstr *, 32> CoveredSetup;
    for (const AffineValue &Value : Values)
      CoveredSetup.insert_range(Value.Setup);

    SmallDenseSet<Register, 16> CheckedRegs;

    Register Base = Values.front().Base;
    int64_t PreviousOffset = Values.front().Offset;

    for (unsigned I = 1; I != Values.size(); ++I) {
      int64_t Delta = Values[I].Offset - PreviousOffset;
      if (Values[I].Base != Base ||
          Values[I].Scale != Values.front().Scale || !isInt<8>(Delta)) {
        Valid = false;
        break;
      }

      Register Reg = Values[I].Reg;

      // The common affine root is intentionally retained. Its other uses include
      // the setup instructions that originally materialized the derived values.
      if (Reg != Base && CheckedRegs.insert(Reg).second) {
        for (MachineOperand &Use : MRI.use_nodbg_operands(Reg)) {
          MachineInstr *User = Use.getParent();

          bool CoveredServiceUse =
              llvm::any_of(Run, [&](const ServiceUse &Service) {
                return Service.MI == User &&
                      Service.OperandNo == Use.getOperandNo();
              });

          if (!CoveredServiceUse && !CoveredSetup.contains(User)) {
            Valid = false;
            break;
          }
        }
      }

      if (!Valid)
        break;

      PreviousOffset = Values[I].Offset;
    }
    if (!Valid)
      continue;

    auto SetupPoint = [&](const ServiceUse &Use) {
      return SetupPoints ? SetupPoints->lookup(Use.MI) : Use.MI;
    };
    if (MDT) {
      MachineInstr *Def = MRI.getVRegDef(Values.front().Reg);
      if (!Def || !MDT->dominates(Def, SetupPoint(Run.front())))
        continue;
    }

    const TargetRegisterClass *FixedRC =
        getAVMFixedRegisterClass(Seed.Input->PhysReg, AVMServiceValueKind::I16);
    assert(FixedRC && "invalid fixed i16 service register");
    MachineInstr *FirstSetup = SetupPoint(Run.front());
    MachineBasicBlock &MBB = *FirstSetup->getParent();
    Register Chain = MRI.createVirtualRegister(FixedRC);
    BuildMI(MBB, FirstSetup->getIterator(),
            Run.front().MI->getDebugLoc(),
            TII.get(TargetOpcode::COPY), Chain)
        .addReg(Values.front().Reg);
    Run.front().MI->getOperand(Run.front().OperandNo).setReg(Chain);
    Run.front().MI->getOperand(Run.front().OperandNo).setIsKill(false);

    PreviousOffset = Values.front().Offset;
    for (unsigned I = 1; I != Values.size(); ++I) {
      int64_t Delta = Values[I].Offset - PreviousOffset;
      if (Delta) {
        Register Next = MRI.createVirtualRegister(FixedRC);
        MachineInstr *At = SetupPoint(Run[I]);
        BuildMI(*At->getParent(), At->getIterator(), Run[I].MI->getDebugLoc(),
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

struct ConditionalService {
  MachineBasicBlock *Next;
  MachineInstr *Service;
};

// Extend an affine service region through a conditional block when its
// optional service immediately reconverges with the skip path.
// The ABI setup is placed before the condition's terminator, so later inputs
// can advance the carrier whether or not an earlier service executed.
static std::optional<ConditionalService>
getConditionalService(MachineBasicBlock &Test, ArrayRef<MCPhysReg> RequiredRegs,
                       const TargetRegisterInfo &TRI) {
  if (Test.succ_size() != 2)
    return std::nullopt;
  for (MachineInstr &MI : Test) {
    if (MI.isDebugInstr())
      continue;
    if (MI.isTerminator()) {
      if (MI.getOpcode() != AVM::BR_CC_PSEUDO &&
          MI.getOpcode() != AVM::JMP_PSEUDO)
        return std::nullopt;
    } else if (isAVMSystemService(MI.getOpcode()) ||
               isRegionBoundary(MI, RequiredRegs, TRI)) {
      return std::nullopt;
    }
    for (const MachineOperand &MO : MI.operands())
      if (MO.isReg() &&
          aliasesRequiredServiceRegister(MO.getReg(), RequiredRegs, TRI))
        return std::nullopt;
  }
  for (MachineBasicBlock *Optional : Test.successors()) {
    if (Optional == &Test || Optional->pred_size() != 1 ||
        Optional->succ_size() != 1)
      continue;
    MachineBasicBlock *Next = *Optional->succ_begin();
    if (Next == Optional || Next == &Test || Next->pred_size() != 2 ||
        !Test.isSuccessor(Next))
      continue;
    MachineInstr *Service = nullptr;
    bool Valid = true;
    for (MachineInstr &MI : *Optional) {
      if (MI.isDebugInstr() || MI.getOpcode() == AVM::JMP_PSEUDO)
        continue;
      if (isAVMSystemService(MI.getOpcode())) {
        if (Service)
          Valid = false;
        Service = &MI;
      } else if (isRegionBoundary(MI, RequiredRegs, TRI)) {
        Valid = false;
      } else {
        for (const MachineOperand &MO : MI.operands())
          if (MO.isReg() && aliasesRequiredServiceRegister(
                                MO.getReg(), RequiredRegs, TRI))
            Valid = false;
      }
    }
    if (Valid && Service)
      return ConditionalService{Next, Service};
  }
  return std::nullopt;
}

static bool chainConditionalServiceInputs(MachineFunction &MF,
                                          ArrayRef<MCPhysReg> RequiredRegs,
                                          MachineRegisterInfo &MRI,
                                          MachineDominatorTree &MDT,
                                          const TargetInstrInfo &TII,
                                          const TargetRegisterInfo &TRI) {
  SmallPtrSet<MachineBasicBlock *, 16> Handled;
  bool Changed = false;
  for (MachineBasicBlock &Start : MF) {
    if (Handled.contains(&Start))
      continue;
    SmallVector<MachineInstr *, 16> Services;
    DenseMap<MachineInstr *, MachineInstr *> SetupPoints;
    SmallPtrSet<MachineBasicBlock *, 16> Seen;
    MachineBasicBlock *Test = &Start;
    while (Services.size() != 16 && Seen.insert(Test).second) {
      auto Conditional = getConditionalService(*Test, RequiredRegs, TRI);
      if (!Conditional)
        break;
      Services.push_back(Conditional->Service);
      SetupPoints[Conditional->Service] = &*Test->getFirstTerminator();
      Handled.insert(Test);
      Test = Conditional->Next;
    }
    if (Services.size() >= 2)
      Changed |= chainAffineInputs(Services, MRI, TII, TRI, &MDT, &SetupPoints);
  }
  return Changed;
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

      int TiedOperandNo = getAVMServiceInputOperandIndex(
          Info, static_cast<unsigned>(Output.TiedLogicalInput));
      assert(TiedOperandNo >= 0 && "tied service input must be explicit");

      SmallVector<MachineOperand *, 4> CoveredUses;
      SmallVector<MachineOperand *, 4> EscapingUses;
      for (MachineOperand &Use : MRI.use_nodbg_operands(GeneralResult)) {
        // If the tied input already uses the same virtual register as the
        // result, it is not an escaping use of the produced result.
        if (Use.getParent() == MI &&
            Use.getOperandNo() == static_cast<unsigned>(TiedOperandNo))
          continue;

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

      // Always expose the tied ABI register to register allocation, including
      // when the result is dead.  The input carrier is intentionally mergeable:
      // AVMRegisterInfo permits only a one-use general source to coalesce into
      // the fixed carrier.
      Register InputReg = MI->getOperand(TiedOperandNo).getReg();
      const TargetRegisterClass *FixedRC =
          getAVMFixedRegisterClass(Output.PhysReg, Output.Kind);
      assert(FixedRC && "invalid tied service output register");
      Register FixedInput = MRI.createVirtualRegister(FixedRC);
      BuildMI(*MI->getParent(), MI->getIterator(), MI->getDebugLoc(),
              TII.get(TargetOpcode::COPY), FixedInput)
          .addReg(InputReg);
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
  SmallVector<Register, 4> OriginalValues;
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

// Treat separately selected copies of the same constant or program address as
// the same service value.  This allows a fixed carrier to remain resident even
// when SelectionDAG produced distinct virtual registers for equivalent
// materializations.
static bool areEquivalentServiceValues(Register LHS, Register RHS,
                                       MachineRegisterInfo &MRI) {
  if (LHS == RHS)
    return true;
  if (!LHS.isVirtual() || !RHS.isVirtual())
    return false;

  MachineInstr *LHSDef = MRI.getVRegDef(LHS);
  MachineInstr *RHSDef = MRI.getVRegDef(RHS);
  if (!LHSDef || !RHSDef || LHSDef->getOpcode() != RHSDef->getOpcode() ||
      !isConstantMaterialization(*LHSDef) ||
      !isConstantMaterialization(*RHSDef) ||
      LHSDef->getNumOperands() != RHSDef->getNumOperands())
    return false;

  for (unsigned I = 0; I != LHSDef->getNumOperands(); ++I) {
    const MachineOperand &LHSOp = LHSDef->getOperand(I);
    const MachineOperand &RHSOp = RHSDef->getOperand(I);
    if (I == 0 && LHSOp.isReg() && LHSOp.isDef() && RHSOp.isReg() &&
        RHSOp.isDef())
      continue;
    if (!LHSOp.isIdenticalTo(RHSOp))
      return false;
  }
  return true;
}

static bool isImmediateMaterialization(Register Value,
                                       MachineRegisterInfo &MRI) {
  if (!Value.isVirtual())
    return false;
  MachineInstr *Def = MRI.getVRegDef(Value);
  if (!Def)
    return false;
  switch (Def->getOpcode()) {
  case AVM::LDI8_PSEUDO:
  case AVM::LDI16_PSEUDO:
    return Def->getNumExplicitOperands() == 2 &&
           Def->getOperand(1).isImm();
  case AVM::LDI32_PSEUDO:
    return Def->getNumExplicitOperands() == 3 &&
           Def->getOperand(1).isImm() && Def->getOperand(2).isImm();
  default:
    return false;
  }
}

static bool materializeImmediateCarrier(
    Register Value, Register Dest, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator InsertAt, const DebugLoc &DL,
    MachineRegisterInfo &MRI, const TargetInstrInfo &TII) {
  if (!isImmediateMaterialization(Value, MRI))
    return false;

  MachineInstr *Def = MRI.getVRegDef(Value);
  MachineInstrBuilder MIB =
      BuildMI(MBB, InsertAt, DL, TII.get(Def->getOpcode()), Dest);
  for (unsigned I = 1; I != Def->getNumExplicitOperands(); ++I)
    MIB.add(Def->getOperand(I));
  return true;
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

static bool loopConflictsWithCarrier(
    MachineLoop &Loop, MCPhysReg PhysReg, Register Value,
    MachineRegisterInfo &MRI, const TargetRegisterInfo &TRI) {
  for (MachineBasicBlock *MBB : Loop.blocks()) {
    for (MachineInstr &MI : *MBB) {
      const AVMSystemServiceInfo *Info =
          getAVMSystemServiceInfo(MI.getOpcode());
      if (!Info)
        continue;

      // A service result in this register destroys a loop-carried input.
      for (const AVMServiceOutputInfo &Output : Info->Outputs)
        if (TRI.regsOverlap(Output.PhysReg, PhysReg))
          return true;

      unsigned OperandNo = Info->Outputs.size();
      for (const AVMServiceInputInfo &Input : Info->Inputs) {
        if (!Input.PassToMachine)
          continue;

        const MachineOperand &MO = MI.getOperand(OperandNo++);
        if (!TRI.regsOverlap(Input.PhysReg, PhysReg))
          continue;

        if (!MO.isReg() ||
            !areEquivalentServiceValues(MO.getReg(), Value, MRI))
          return true;
      }
    }
  }

  return false;
}

static bool createCommonCarriers(ArrayRef<MachineInstr *> Services,
                                 MachineRegisterInfo &MRI, MachineLoopInfo &MLI,
                                 MachineDominatorTree &MDT,
                                 const TargetInstrInfo &TII,
                                 const TargetRegisterInfo &TRI) {
  SmallVector<ServiceUse, 8> AllUses = collectServiceUses(Services);
  SmallVector<CarrierGroup, 8> Groups;
  for (const ServiceUse &Use : AllUses) {
    Register Value = Use.MI->getOperand(Use.OperandNo).getReg();
    if (!Value.isVirtual())
      continue;
    CarrierGroup *Group = nullptr;
    for (CarrierGroup &Candidate : llvm::reverse(Groups)) {
      if (Candidate.PhysReg != Use.Input->PhysReg ||
          Candidate.Kind != Use.Input->Kind ||
          !areEquivalentServiceValues(Candidate.Value, Value, MRI))
        continue;
      unsigned PreviousIndex = Candidate.Uses.back().ServiceIndex;
      if (!hasOutputBetween(Services, PreviousIndex, Use.ServiceIndex,
                            Use.Input->PhysReg, TRI) &&
          !hasConflictingInputBetween(Services, PreviousIndex,
                                      Use.ServiceIndex, Use.Input->PhysReg,
                                      Value, MRI, TRI))
        Group = &Candidate;
      break;
    }
    if (!Group) {
      Groups.push_back(
          {Use.Input->PhysReg, Use.Input->Kind, Value, {Use}, {Value}});
      continue;
    }
    Group->Uses.push_back(Use);
    Group->OriginalValues.push_back(Value);
  }

  bool Changed = false;
  for (CarrierGroup &Group : Groups) {
    // An earlier persistent carrier may already have replaced this group's
    // service operands with an equivalent value home in another fixed register.
    bool ReplacedByEarlierCarrier = false;
    for (auto [I, Use] : llvm::enumerate(Group.Uses)) {
      const MachineOperand &MO = Use.MI->getOperand(Use.OperandNo);
      if (!MO.isReg() || MO.getReg() != Group.OriginalValues[I]) {
        ReplacedByEarlierCarrier = true;
        break;
      }
    }
    if (ReplacedByEarlierCarrier)
      continue;

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

      if (LoopInvariant &&
          loopConflictsWithCarrier(*Loop, Group.PhysReg, Group.Value, MRI, TRI))
        LoopInvariant = false;

      if (LoopInvariant)
        Preheader = Loop->getLoopPreheader();
      if (!Preheader)
        LoopInvariant = false;
    }
    // A one-use loop-invariant immediate is cheaper and much less disruptive
    // when rematerialized in the required register at the service.  Keeping it
    // in a singleton fixed class across the loop gives greedy RA a long, cheap
    // interval that it may evict and spill even when the physical register is
    // ultimately available.  Retain a persistent immediate carrier only when
    // it has multiple uses in this register or can also serve an equivalent
    // operand in another fixed service register.
    bool HasCrossRegisterReuse = false;
    if (LoopInvariant && isImmediateMaterialization(Group.Value, MRI)) {
      HasCrossRegisterReuse = llvm::any_of(AllUses, [&](const ServiceUse &Other) {
        if (Other.Input->Kind != Group.Kind ||
            Other.Input->PhysReg == Group.PhysReg ||
            !Loop->contains(Other.MI->getParent()))
          return false;
        const MachineOperand &OtherMO =
            Other.MI->getOperand(Other.OperandNo);
        return OtherMO.isReg() &&
               areEquivalentServiceValues(OtherMO.getReg(), Group.Value, MRI);
      });
    }

    if (Group.Uses.size() < 2 &&
        (!LoopInvariant ||
         (isImmediateMaterialization(Group.Value, MRI) &&
          !HasCrossRegisterReuse)))
      continue;

    MachineBasicBlock *InsertBlock = UseBlock;
    MachineBasicBlock::iterator InsertAt = Group.Uses.front().MI->getIterator();
    if (LoopInvariant) {
      InsertBlock = Preheader;
      InsertAt = Preheader->getFirstTerminator();
      if (Def && Loop->contains(Def) &&
          !isImmediateMaterialization(Group.Value, MRI)) {
        Def->removeFromParent();
        Preheader->insert(InsertAt, Def);
      }
    }

    Register Carrier = MRI.createVirtualRegister(FixedRC);
    if (!materializeImmediateCarrier(
            Group.Value, Carrier, *InsertBlock, InsertAt,
            Group.Uses.front().MI->getDebugLoc(), MRI, TII))
      BuildMI(*InsertBlock, InsertAt, Group.Uses.front().MI->getDebugLoc(),
              TII.get(TargetOpcode::COPY), Carrier)
          .addReg(Group.Value);

    for (const ServiceUse &Use : Group.Uses) {
      Use.MI->getOperand(Use.OperandNo).setReg(Carrier);
      Use.MI->getOperand(Use.OperandNo).setIsKill(false);
    }

    // A loop-carried fixed value that survived loopConflictsWithCarrier is a
    // safe value home for equivalent operands in other service registers.
    // Reusing it avoids keeping the original general value live across the loop;
    // createLocalServiceCarriers will add only the required fixed-to-fixed copy.
    if (LoopInvariant) {
      for (const ServiceUse &Other : AllUses) {
        if (Other.Input->Kind != Group.Kind ||
            !Loop->contains(Other.MI->getParent()) ||
            !MDT.dominates(InsertBlock, Other.MI->getParent()))
          continue;

        MachineOperand &OtherMO =
            Other.MI->getOperand(Other.OperandNo);
        if (!OtherMO.isReg() ||
            !areEquivalentServiceValues(OtherMO.getReg(), Group.Value, MRI))
          continue;

        OtherMO.setReg(Carrier);
        OtherMO.setIsKill(false);
      }
    }
    Changed = true;
  }
  return Changed;
}

// Give every service operand that was not handled by an affine, common, or
// tied-result transform a short fixed-register carrier.  This makes the ABI
// occupancy visible to greedy RA while keeping semantic values in general
// classes outside the service instruction.
static bool createLocalServiceCarriers(ArrayRef<MachineInstr *> Services,
                                       MachineRegisterInfo &MRI,
                                       const TargetInstrInfo &TII) {
  bool Changed = false;
  for (MachineInstr *MI : Services) {
    const AVMSystemServiceInfo &Info =
        getRequiredAVMSystemServiceInfo(MI->getOpcode());

    for (const AVMServiceOutputInfo &Output : Info.Outputs) {
      assert(Output.ResultIndex < Info.Outputs.size() &&
             "invalid service result index");
      MachineOperand &OutputMO = MI->getOperand(Output.ResultIndex);
      if (!OutputMO.isReg() || !OutputMO.getReg().isVirtual())
        continue;

      Register GeneralResult = OutputMO.getReg();
      const TargetRegisterClass *FixedRC =
          getAVMFixedRegisterClass(Output.PhysReg, Output.Kind);
      assert(FixedRC && "invalid fixed service output register");
      if (MRI.getRegClass(GeneralResult) == FixedRC)
        continue;

      bool HasUses = !MRI.use_nodbg_empty(GeneralResult);
      Register FixedResult = MRI.createVirtualRegister(FixedRC);
      OutputMO.setReg(FixedResult);
      if (HasUses) {
        MachineInstrBuilder OutCopy =
            BuildMI(*MI->getParent(), std::next(MI->getIterator()),
                    MI->getDebugLoc(), TII.get(TargetOpcode::COPY),
                    GeneralResult)
                .addReg(FixedResult);
        OutCopy->setFlag(MachineInstr::NoMerge);
      }
      Changed = true;
    }

    unsigned OperandNo = Info.Outputs.size();
    for (const AVMServiceInputInfo &Input : Info.Inputs) {
      if (!Input.PassToMachine)
        continue;
      MachineOperand &InputMO = MI->getOperand(OperandNo++);
      if (!InputMO.isReg() || !InputMO.getReg().isVirtual())
        continue;

      Register GeneralInput = InputMO.getReg();
      const TargetRegisterClass *FixedRC =
          getAVMFixedRegisterClass(Input.PhysReg, Input.Kind);
      assert(FixedRC && "invalid fixed service input register");
      if (MRI.getRegClass(GeneralInput) == FixedRC)
        continue;

      const TargetRegisterClass *GeneralRC = MRI.getRegClass(GeneralInput);

      // A local immediate setup has no parallel-copy dependency, so define the
      // architectural ABI register directly.  Keeping even this tiny value in
      // a spillable singleton virtual class lets greedy RA evict it and emit a
      // store/reload pair.  An explicit physical definition is the same model
      // used by ordinary fixed-register argument setup: RA must route flexible
      // live ranges around it, but cannot spill the ABI setup itself.
      //
      // Do this only for immediate materializations.  General and fixed-source
      // copies remain virtual until RA so AVMExpandSystemServices can resolve
      // swaps and other parallel-copy cycles safely.
      if (!isAVMFixedServiceRegisterClass(GeneralRC) &&
          materializeImmediateCarrier(
              GeneralInput, Input.PhysReg, *MI->getParent(),
              MI->getIterator(), MI->getDebugLoc(), MRI, TII)) {
        InputMO.setReg(Input.PhysReg);
        InputMO.setIsKill(false);
        Changed = true;
        continue;
      }

      Register FixedInput = MRI.createVirtualRegister(FixedRC);
      // Keep values already resident in another fixed carrier as copies:
      // an upper-register MOV is cheaper than reloading the immediate.
      BuildMI(*MI->getParent(), MI->getIterator(), MI->getDebugLoc(),
              TII.get(TargetOpcode::COPY), FixedInput)
          .addReg(GeneralInput);
      InputMO.setReg(FixedInput);
      InputMO.setIsKill(false);
      Changed = true;
    }
  }
  return Changed;
}

static bool eraseDeadImmediateMaterializations(MachineFunction &MF,
                                               MachineRegisterInfo &MRI) {
  bool Changed = false;
  for (MachineBasicBlock &MBB : MF) {
    for (auto It = MBB.begin(); It != MBB.end();) {
      MachineInstr &MI = *It++;
      unsigned Opcode = MI.getOpcode();
      if (Opcode != AVM::LDI8_PSEUDO && Opcode != AVM::LDI16_PSEUDO &&
          Opcode != AVM::LDI32_PSEUDO)
        continue;
      Register Def = MI.getOperand(0).getReg();
      if (!Def.isVirtual() || !MRI.use_nodbg_empty(Def))
        continue;
      MI.eraseFromParent();
      Changed = true;
    }
  }
  return Changed;
}

static bool fixedServiceClassesOverlap(const TargetRegisterClass *LHS,
                                       const TargetRegisterClass *RHS,
                                       const TargetRegisterInfo &TRI) {
  for (MCPhysReg LHSReg : LHS->getRegisters())
    for (MCPhysReg RHSReg : RHS->getRegisters())
      if (TRI.regsOverlap(LHSReg, RHSReg))
        return true;
  return false;
}

static bool isPhysicalRegisterCopy(const MachineInstr &MI) {
  if (!MI.isCopy() || MI.getNumExplicitOperands() < 2)
    return false;

  const MachineOperand &Src = MI.getOperand(1);
  return Src.isReg() && Src.getReg().isPhysical();
}

// Folding a general source into a singleton carrier extends the singleton live
// range back to the source definition.  Restrict deterministic folding to
// adjacent setup sequences: only debug instructions and non-overlapping fixed
// carrier copies may appear between the definition and carrier copy.
static bool canFoldOneUseCarrierCopy(
    const MachineInstr &Copy, Register Src,
    const TargetRegisterClass *FixedRC, MachineRegisterInfo &MRI,
    const TargetRegisterInfo &TRI) {
  MachineInstr *Def = MRI.getVRegDef(Src);
  if (!Def || Def->getParent() != Copy.getParent())
    return false;
  
  if (isPhysicalRegisterCopy(*Def))
    return false;

  auto CopyIt = Copy.getIterator();
  auto End = Def->getParent()->end();
  for (auto It = std::next(Def->getIterator()); It != CopyIt; ++It) {
    if (It == End)
      return false;
    if (It->isDebugInstr())
      continue;
    if (!It->isCopy() || It->getNumExplicitOperands() < 2 ||
        !It->getOperand(0).isReg() ||
        !It->getOperand(0).getReg().isVirtual())
      return false;

    const TargetRegisterClass *OtherRC =
        MRI.getRegClass(It->getOperand(0).getReg());
    if (!isAVMFixedServiceRegisterClass(OtherRC) ||
        fixedServiceClassesOverlap(FixedRC, OtherRC, TRI))
      return false;
  }
  return true;
}

// Deterministically fold short one-use setup values into their singleton
// service classes.  The ordinary coalescer remains responsible for longer
// common carriers where interference analysis is required.
static bool foldOneUseCarrierCopies(MachineFunction &MF,
                                    MachineRegisterInfo &MRI,
                                    const TargetRegisterInfo &TRI) {
  bool Changed = false;
  for (MachineBasicBlock &MBB : MF) {
    for (auto It = MBB.begin(); It != MBB.end();) {
      MachineInstr &MI = *It++;
      if (!MI.isCopy() || MI.getFlag(MachineInstr::NoMerge) ||
          MI.getNumExplicitOperands() < 2)
        continue;

      MachineOperand &DstMO = MI.getOperand(0);
      MachineOperand &SrcMO = MI.getOperand(1);
      if (!DstMO.isReg() || !SrcMO.isReg())
        continue;
      Register Dst = DstMO.getReg();
      Register Src = SrcMO.getReg();
      if (!Dst.isVirtual() || !Src.isVirtual())
        continue;

      const TargetRegisterClass *FixedRC = MRI.getRegClass(Dst);
      if (!isAVMFixedServiceRegisterClass(FixedRC) ||
          isAVMFixedServiceRegisterClass(MRI.getRegClass(Src)))
        continue;

      MachineOperand *OnlyUse = MRI.getOneNonDBGUse(Src);
      if (!OnlyUse || OnlyUse->getParent() != &MI ||
          OnlyUse->getOperandNo() != 1 ||
          !canFoldOneUseCarrierCopy(MI, Src, FixedRC, MRI, TRI))
        continue;

      if (!MRI.constrainRegClass(Src, FixedRC))
        continue;

      MRI.replaceRegWith(Dst, Src);
      MRI.clearKillFlags(Src);
      MI.eraseFromParent();
      Changed = true;
    }
  }
  return Changed;
}

// Read a short-lived service result directly as the non-destructive operand
// of an integer sum. The accumulator stays general, so its lifetime cannot
// absorb the fixed ABI carrier or extend it across the next service call.
static bool foldLocalServiceResultAdds(MachineFunction &MF,
                                       MachineRegisterInfo &MRI,
                                       const TargetInstrInfo &TII) {
  bool Changed = false;
  for (MachineBasicBlock &MBB : MF)
    for (auto It = MBB.begin(); It != MBB.end();) {
      MachineInstr &Copy = *It++;
      if (!Copy.isFullCopy())
        continue;
      Register Dst = Copy.getOperand(0).getReg();
      Register Src = Copy.getOperand(1).getReg();
      if (!Dst.isVirtual() || !Src.isVirtual() || !MRI.hasOneUse(Dst) ||
          !AVM::GPR16RegClass.hasSubClassEq(MRI.getRegClass(Dst)) ||
          isAVMFixedServiceRegisterClass(MRI.getRegClass(Dst)))
        continue;
      const TargetRegisterClass *FixedRC = MRI.getRegClass(Src);
      MachineInstr *Def = MRI.getVRegDef(Src);
      if (!isAVMFixedServiceRegisterClass(FixedRC) ||
          !AVM::GPR16RegClass.hasSubClassEq(FixedRC) ||
          FixedRC->getNumRegs() != 1 || !Def ||
          Def->getParent() != &MBB ||
          !getAVMSystemServiceInfo(Def->getOpcode()))
        continue;
      MachineOperand *Use = MRI.getOneNonDBGUse(Dst);
      if (!Use || Use->getSubReg() ||
          (Use->getOperandNo() != 1 && Use->getOperandNo() != 2))
        continue;
      MachineInstr &Add = *Use->getParent();
      if (Add.getOpcode() != AVM::ADD16_PSEUDO || Add.getParent() != &MBB)
        continue;
      Register Result = Add.getOperand(0).getReg();
      if (!Result.isVirtual() || Add.getOperand(0).getSubReg() ||
          !AVM::GPR16RegClass.hasSubClassEq(MRI.getRegClass(Result)) ||
          isAVMFixedServiceRegisterClass(MRI.getRegClass(Result)))
        continue;
      unsigned OtherNo = Use->getOperandNo() == 1 ? 2 : 1;
      Register Other = Add.getOperand(OtherNo).getReg();
      if (!Other.isVirtual() || Add.getOperand(OtherNo).getSubReg() ||
          isAVMFixedServiceRegisterClass(MRI.getRegClass(Other)))
        continue;
      // Retaining a fixed result across even a few independent operations
      // can evict a more valuable general value from an upper register.
      // Limit this fold to an immediate consumer rather than extending the
      // carrier through a deferred reduction tree or another fixed use.
      auto Next = std::next(Copy.getIterator());
      while (Next != MBB.end() && Next->isMetaInstruction())
        ++Next;
      if (Next == MBB.end() || &*Next != &Add)
        continue;
      if (OtherNo == 2 && !TII.commuteInstruction(Add))
        continue;
      Add.getOperand(2).setReg(Src);
      Add.getOperand(2).setIsKill(false);
      MRI.clearKillFlags(Src);
      Add.setDesc(TII.get(AVM::ADD16_FIXED_RHS_PSEUDO));
      Copy.eraseFromParent();
      Changed = true;
    }
  return Changed;
}

// With several byte products, keeping an accumulator in the compact bank
// through every product can force two temporary spills. Tie the first sum to
// its consumed product instead, accepting a short backedge copy so the old
// accumulator can use one spill/reload while the products occupy that bank.
static bool tieProductSums(MachineFunction &MF, MachineRegisterInfo &MRI,
                           MachineLoopInfo &MLI, const TargetInstrInfo &TII,
                           const TargetRegisterInfo &TRI) {
  bool Changed = false;
  for (MachineBasicBlock &MBB : MF) {
    MachineLoop *Loop = MLI.getLoopFor(&MBB);
    if (!Loop || Loop->getHeader() != &MBB || Loop->getNumBlocks() != 1)
      continue;
    unsigned Products = 0;
    bool ByteProduct = false, IntegerOnly = true;
    for (const MachineInstr &MI : MBB) {
      if (MI.isCall() || MI.isInlineAsm() ||
          MI.modifiesRegister(AVM::R4R5, &TRI) ||
          MI.modifiesRegister(AVM::R6R7, &TRI))
        IntegerOnly = false;
      switch (MI.getOpcode()) {
      case AVM::MUL8_PSEUDO:
      case AVM::MULU8W_PSEUDO:
      case AVM::MULU8W_FLEX_PSEUDO:
      case AVM::MULS8W_PSEUDO:
      case AVM::MULSU8W_PSEUDO:
        ByteProduct = true;
        [[fallthrough]];
      case AVM::MUL16_PSEUDO:
        ++Products;
        break;
      }
    }
    if (!IntegerOnly || !ByteProduct || Products < 3)
      continue;
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() != AVM::ADD16_PSEUDO)
        continue;
      for (unsigned PhiNo : {1u, 2u}) {
        Register Acc = MI.getOperand(PhiNo).getReg();
        Register Product = MI.getOperand(3 - PhiNo).getReg();
        if (!Acc.isVirtual() || !Product.isVirtual() ||
            MI.getOperand(PhiNo).getSubReg() ||
            MI.getOperand(3 - PhiNo).getSubReg() ||
            !AVM::GPR16RegClass.hasSubClassEq(MRI.getRegClass(Acc)) ||
            !MRI.hasOneNonDBGUse(Acc) || !MRI.hasOneNonDBGUse(Product))
          continue;
        const MachineInstr *Phi = MRI.getUniqueVRegDef(Acc);
        const MachineInstr *Def = MRI.getUniqueVRegDef(Product);
        if (!Phi || !Phi->isPHI() || Phi->getParent() != &MBB || !Def ||
            Def->getParent() != &MBB ||
            Def->getOpcode() != AVM::MUL16_PSEUDO)
          continue;
        if (PhiNo == 1 && !TII.commuteInstruction(MI))
          continue;
        MI.setDesc(TII.get(AVM::ADD16_FIXED_RHS_PSEUDO));
        Changed = true;
        break;
      }
    }
  }
  return Changed;
}

// A wide accumulation need not overwrite its old PHI. Consumed loads and pairs
// assembled by compact word operations already have a useful result pair;
// carry the sum through that pair instead of copying it to the accumulator.
static bool tieWideInputSums(MachineFunction &MF, MachineRegisterInfo &MRI,
                             MachineLoopInfo &MLI, const TargetInstrInfo &TII) {
  bool Changed = false;
  for (MachineBasicBlock &MBB : MF) {
    MachineLoop *Loop = MLI.getLoopFor(&MBB);
    if (!Loop || Loop->getHeader() != &MBB || Loop->getNumBlocks() != 1)
      continue;
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() != AVM::ADD32_PSEUDO)
        continue;
      for (unsigned PhiNo : {1u, 2u}) {
        Register Acc = MI.getOperand(PhiNo).getReg();
        Register Input = MI.getOperand(3 - PhiNo).getReg();
        if (!Acc.isVirtual() || !Input.isVirtual() ||
            MI.getOperand(PhiNo).getSubReg() ||
            MI.getOperand(3 - PhiNo).getSubReg() ||
            !MRI.hasOneNonDBGUse(Acc) || !MRI.hasOneNonDBGUse(Input))
          continue;
        const MachineInstr *Phi = MRI.getUniqueVRegDef(Acc);
        const MachineInstr *Def = MRI.getUniqueVRegDef(Input);
        if (!Phi || !Phi->isPHI() || Phi->getParent() != &MBB || !Def ||
            Def->getParent() != &MBB)
          continue;
        bool CompactPair = Def->getOpcode() == AVM::BUILD_PAIR_PSEUDO;
        if (CompactPair)
          for (unsigned WordNo : {1u, 2u}) {
            Register Word = Def->getOperand(WordNo).getReg();
            const MachineInstr *WordDef =
                Word.isVirtual() ? MRI.getUniqueVRegDef(Word) : nullptr;
            if (!WordDef || WordDef->getParent() != &MBB ||
                WordDef->getOpcode() != AVM::OR16_PSEUDO ||
                !MRI.hasOneNonDBGUse(Word) ||
                !WordDef->getOperand(1).getReg().isVirtual() ||
                !WordDef->getOperand(2).getReg().isVirtual() ||
                !AVM::UpperGPR16RegClass.hasSubClassEq(
                    MRI.getRegClass(WordDef->getOperand(1).getReg())) ||
                !AVM::UpperGPR16RegClass.hasSubClassEq(
                    MRI.getRegClass(WordDef->getOperand(2).getReg())))
              CompactPair = false;
          }
        if (Def->getOpcode() != AVM::LOAD32_PSEUDO && !CompactPair)
          continue;
        if (PhiNo == 1 && !TII.commuteInstruction(MI))
          continue;
        MI.setDesc(TII.get(AVM::ADD32_FIXED_RHS_PSEUDO));
        Changed = true;
        break;
      }
    }
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
      Changed |= chainConditionalServiceInputs(MF, RequiredRegs, MRI, MDT,
                                               TII, TRI);
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
            Changed |= createLocalServiceCarriers(Services, MRI, TII);
          }
          RegionBegin = RegionEnd;
        }
      }
      Changed |= foldOneUseCarrierCopies(MF, MRI, TRI);
      Changed |= foldLocalServiceResultAdds(MF, MRI, TII);
      Changed |= tieProductSums(MF, MRI, MLI, TII, TRI);
      Changed |= tieWideInputSums(MF, MRI, MLI, TII);
      Changed |= eraseDeadImmediateMaterializations(MF, MRI);
    }

    // Avoid aggressive local one-use rematerialization perturbing fixed-carrier
    // allocation. An invariant with one static loop use is still worth
    // rematerializing: otherwise that use can reload a spilled constant on
    // every iteration.
    for (MachineBasicBlock &MBB : MF)
      for (MachineInstr &MI : MBB) {
        unsigned Opcode = MI.getOpcode();
        if (Opcode != AVM::LDI8_PSEUDO && Opcode != AVM::LDI16_PSEUDO &&
            Opcode != AVM::LDI32_PSEUDO)
          continue;
        Register Def = MI.getOperand(0).getReg();
        // A shared service carrier may hide multiple uses behind one COPY.
        // Follow that carrier before deciding whether rematerialization pays.
        while (Def.isVirtual()) {
          MachineOperand *Use = MRI.getOneNonDBGUse(Def);
          if (!Use || Use->getSubReg())
            break;
          MachineInstr &Copy = *Use->getParent();
          if (!Copy.isFullCopy() || Use->getOperandNo() != 1 ||
              !Copy.getOperand(0).getReg().isVirtual())
            break;
          Def = Copy.getOperand(0).getReg();
        }
        if (!Def.isVirtual() || MRI.use_nodbg_empty(Def))
          continue;
        if (Opcode == AVM::LDI8_PSEUDO &&
            AVM::GPR16RegClass.hasSubClassEq(MRI.getRegClass(Def))) {
          const MachineBasicBlock *UseBlock = nullptr;
          unsigned BitwiseUses = 0;
          bool Eligible = true;
          for (const MachineInstr &Use : MRI.use_nodbg_instructions(Def)) {
            unsigned UseOpcode = Use.getOpcode();
            if ((UseOpcode != AVM::AND16_PSEUDO &&
                 UseOpcode != AVM::OR16_PSEUDO &&
                 UseOpcode != AVM::XOR16_PSEUDO) ||
                (UseBlock && UseBlock != Use.getParent())) {
              Eligible = false;
              break;
            }
            UseBlock = Use.getParent();
            ++BitwiseUses;
          }
          MachineLoop *UseLoop = UseBlock ? MLI.getLoopFor(UseBlock) : nullptr;
          bool InvariantMask = UseLoop && !UseLoop->contains(MI.getParent());
          // Many unrolled uses can keep a mask resident in the cold bank;
          // forcing it upper instead recreates it for each short expression.
          if (Eligible && BitwiseUses <= 3 &&
              (BitwiseUses >= 3 || InvariantMask) && UseLoop &&
              llvm::none_of(*UseBlock, [&](const MachineInstr &I) {
                return I.isCall() || I.isInlineAsm() ||
                       I.modifiesRegister(AVM::R4R5, &TRI) ||
                       I.modifiesRegister(AVM::R6R7, &TRI);
              })) {
            // A resident invariant mask saves a full bitwise encoding on every
            // iteration; several local uses can amortize materialization too.
            // Favor the compact bank for these integer-only loop masks.
            if (MRI.constrainRegClass(Def, &AVM::UpperGPR16RegClass))
              Changed = true;
          }
        }
        if (MachineOperand *Use = MRI.getOneNonDBGUse(Def)) {
          MachineInstr *User = Use->getParent();
          MachineLoop *UseLoop = MLI.getLoopFor(User->getParent());
          if (!UseLoop)
            continue;
          if (!UseLoop->contains(MI.getParent())) {
            auto WordCost = [](const MachineOperand &MO) {
              uint16_t Bits = MO.getImm();
              return AVM::getFixedCycles(
                  !Bits ? AVM::AVMCostKind::ClrUpper
                        : isUInt<8>(Bits) ? AVM::AVMCostKind::Ldi8Upper
                                         : AVM::AVMCostKind::Ldi16Upper);
            };
            // Full-width nonzero words can cost more to materialize than
            // reloading a short stack slot. Keep those invariants available
            // to allocation instead of forcing rematerialization.
            unsigned Words = Opcode == AVM::LDI32_PSEUDO ? 2 : 1;
            unsigned Cost = WordCost(MI.getOperand(1));
            if (Words == 2)
              Cost += WordCost(MI.getOperand(2));
            if (Cost > Words * AVM::getFixedCycles(AVM::AVMCostKind::LdSp16Short))
              continue;
          }
          if (UseLoop->contains(MI.getParent())) {
            if (User->isPHI() || MI.getParent() != User->getParent())
              continue;
            // A loop-local constant selected before other fixed-register
            // operations can also be evicted before its sole use. Let the
            // allocator recreate it at that use instead of spilling a pair.
            bool CrossesClobber = false;
            for (auto I = std::next(MI.getIterator()), E = User->getIterator();
                 I != E; ++I)
              if (I->isCall() ||
                  llvm::any_of(I->operands(), [&](const MachineOperand &MO) {
                    return MO.isReg() && MO.isDef() &&
                           MO.getReg().isPhysical() &&
                           (TRI.regsOverlap(MO.getReg(), AVM::R4R5) ||
                            TRI.regsOverlap(MO.getReg(), AVM::R6R7));
                  })) {
                CrossesClobber = true;
                break;
              }
            if (!CrossesClobber)
              continue;
          }
        }
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
