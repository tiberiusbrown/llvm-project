//===-- AVMExpandSystemServices.cpp - Expand fixed-register services ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "AVMSystemServiceInfo.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "avm-expand-system-services"
#define PASS_NAME "AVM post-RA system-service expansion"

namespace {

struct PhysAssignment {
  MCPhysReg Dest;
  MCPhysReg Src;
  bool FromStack = false;
};

struct SavedPhysValue {
  MCPhysReg Reg;
  uint64_t Token;
};

static void appendUnits(SmallVectorImpl<MCPhysReg> &Units, MCPhysReg Reg,
                        AVMServiceValueKind Kind,
                        const TargetRegisterInfo &TRI) {
  if (Kind == AVMServiceValueKind::I16) {
    Units.push_back(Reg);
    return;
  }
  MCPhysReg Lo = TRI.getSubReg(Reg, AVM::sub_lo16);
  MCPhysReg Hi = TRI.getSubReg(Reg, AVM::sub_hi16);
  assert(Lo && Hi && "wide service value needs an aligned register pair");
  Units.push_back(Lo);
  Units.push_back(Hi);
}

static void appendAssignmentUnits(SmallVectorImpl<PhysAssignment> &Assignments,
                                  MCPhysReg Dest, MCPhysReg Src,
                                  AVMServiceValueKind Kind,
                                  const TargetRegisterInfo &TRI) {
  SmallVector<MCPhysReg, 2> DestUnits;
  SmallVector<MCPhysReg, 2> SrcUnits;
  appendUnits(DestUnits, Dest, Kind, TRI);
  appendUnits(SrcUnits, Src, Kind, TRI);
  assert(DestUnits.size() == SrcUnits.size());
  for (unsigned I = 0; I != DestUnits.size(); ++I)
    Assignments.push_back({DestUnits[I], SrcUnits[I]});
}

class ParallelCopyResolver {
  MachineFunction &MF;
  const AVMInstrInfo &TII;
  const TargetRegisterInfo &TRI;
  MachineBasicBlock &MBB;
  MachineInstr &Before;
  uint8_t LiveAfter;
  ArrayRef<MCPhysReg> Saved;

  bool isLive(MCPhysReg Reg) const {
    return LiveAfter & (1u << TRI.getEncodingValue(Reg));
  }

  void emitCopy(MCPhysReg Dest, MCPhysReg Src) {
    if (Dest != Src)
      TII.copyPhysReg(MBB, Before.getIterator(), Before.getDebugLoc(), Dest,
                      Src, false);
  }

  MCPhysReg findScratch(ArrayRef<PhysAssignment> Assignments,
                        ArrayRef<MCPhysReg> FinalDestinations) const {
    BitVector Reserved = TRI.getReservedRegs(MF);
    for (MCPhysReg Candidate : {AVM::R0, AVM::R1, AVM::R2, AVM::R3, AVM::R4,
                                AVM::R5, AVM::R6, AVM::R7}) {
      if (Reserved.test(Candidate))
        continue;
      bool IsSource = llvm::any_of(Assignments, [&](const PhysAssignment &A) {
        return !A.FromStack && A.Src == Candidate;
      });
      bool IsDestination = llvm::is_contained(FinalDestinations, Candidate);
      if (IsSource || IsDestination)
        continue;
      if (isLive(Candidate) && !llvm::is_contained(Saved, Candidate))
        continue;
      return Candidate;
    }
    return MCPhysReg();
  }

public:
  ParallelCopyResolver(MachineFunction &MF, const AVMInstrInfo &TII,
                       MachineBasicBlock &MBB, MachineInstr &Before,
                       uint8_t LiveAfter, ArrayRef<MCPhysReg> Saved)
      : MF(MF), TII(TII), TRI(TII.getRegisterInfo()), MBB(MBB), Before(Before),
        LiveAfter(LiveAfter), Saved(Saved) {}

  void emit(SmallVector<PhysAssignment, 8> Assignments) {
    SmallVector<MCPhysReg, 8> FinalDestinations;
    for (const PhysAssignment &A : Assignments)
      FinalDestinations.push_back(A.Dest);
    llvm::erase_if(Assignments, [](const PhysAssignment &A) {
      return !A.FromStack && A.Dest == A.Src;
    });

    while (!Assignments.empty()) {
      auto IsSource = [&](MCPhysReg Reg) {
        return llvm::any_of(Assignments, [&](const PhysAssignment &A) {
          return !A.FromStack && A.Src == Reg;
        });
      };
      auto Ready = llvm::find_if(Assignments, [&](const PhysAssignment &A) {
        return !IsSource(A.Dest);
      });
      if (Ready != Assignments.end()) {
        if (Ready->FromStack) {
          MCPhysReg Dest = Ready->Dest;
          BuildMI(MBB, Before, Before.getDebugLoc(), TII.get(AVM::POP16), Dest);
          Assignments.erase(Ready);
          // Only one logical value occupies the top stack slot. Any duplicate
          // users can now read the register into which it was popped.
          for (PhysAssignment &A : Assignments)
            if (A.FromStack) {
              A.Src = Dest;
              A.FromStack = false;
            }
        } else {
          emitCopy(Ready->Dest, Ready->Src);
          Assignments.erase(Ready);
        }
        continue;
      }

      MCPhysReg CycleValue = Assignments.front().Dest;
      if (MCPhysReg Scratch = findScratch(Assignments, FinalDestinations)) {
        emitCopy(Scratch, CycleValue);
        for (PhysAssignment &A : Assignments)
          if (!A.FromStack && A.Src == CycleValue)
            A.Src = Scratch;
      } else {
        BuildMI(MBB, Before, Before.getDebugLoc(), TII.get(AVM::PUSH16))
            .addReg(CycleValue);
        for (PhysAssignment &A : Assignments)
          if (!A.FromStack && A.Src == CycleValue)
            A.FromStack = true;
      }
    }
  }
};

static uint8_t getLiveMask(const LivePhysRegs &Live,
                           const TargetRegisterInfo &TRI) {
  uint8_t Mask = 0;
  for (MCPhysReg Reg :
       {AVM::R0, AVM::R1, AVM::R2, AVM::R3, AVM::R4, AVM::R5, AVM::R6, AVM::R7})
    if (Live.contains(Reg))
      Mask |= 1u << TRI.getEncodingValue(Reg);
  return Mask;
}

static bool isRegionBarrier(const MachineInstr &MI) {
  return MI.isTerminator() || MI.isCall() || MI.isInlineAsm() ||
         MI.isBarrier() || MI.hasUnmodeledSideEffects() ||
         llvm::any_of(MI.operands(),
                      [](const MachineOperand &MO) { return MO.isRegMask(); });
}

static bool touchesArchitecturalStack(
    const MachineInstr &MI, const TargetRegisterInfo &TRI) {
  return MI.readsRegister(AVM::SP, &TRI) ||
         MI.modifiesRegister(AVM::SP, &TRI);
}

static bool aliasesAnySaved(const MachineInstr &MI,
                            ArrayRef<SavedPhysValue> Saved,
                            const TargetRegisterInfo &TRI) {
  return llvm::any_of(MI.operands(), [&](const MachineOperand &MO) {
    if (!MO.isReg() || !MO.getReg())
      return false;
    return llvm::any_of(Saved, [&](const SavedPhysValue &Value) {
      return TRI.regsOverlap(MO.getReg(), Value.Reg);
    });
  });
}

class AVMExpandSystemServices final : public MachineFunctionPass {
public:
  static char ID;
  AVMExpandSystemServices() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return PASS_NAME; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    const AVMInstrInfo &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
    const TargetRegisterInfo &TRI = TII.getRegisterInfo();
    DenseMap<const MachineInstr *, uint8_t> LiveAfter;

    for (MachineBasicBlock &MBB : MF) {
      LivePhysRegs Live(TRI);
      Live.addLiveOuts(MBB);
      for (MachineInstr &MI : llvm::reverse(MBB)) {
        if (isAVMSystemService(MI.getOpcode()))
          LiveAfter[&MI] = getLiveMask(Live, TRI);
        Live.stepBackward(MI);
      }
    }

    bool Changed = false;
    for (MachineBasicBlock &MBB : MF) {
      uint64_t NextToken = 9;
      uint64_t Tokens[8] = {1, 2, 3, 4, 5, 6, 7, 8};
      SmallVector<SavedPhysValue, 8> ActiveSaved;

      auto TokenIndex = [&](MCPhysReg Reg) {
        unsigned Index = TRI.getEncodingValue(Reg);
        assert(Index < 8 && "expected an architectural 16-bit register");
        return Index;
      };
      auto RestoreSaved = [&](MachineBasicBlock::iterator Before,
                              const DebugLoc &DL) {
        for (const SavedPhysValue &Saved : llvm::reverse(ActiveSaved)) {
          BuildMI(MBB, Before, DL, TII.get(AVM::POP16), Saved.Reg);
          Tokens[TokenIndex(Saved.Reg)] = Saved.Token;
        }
        ActiveSaved.clear();
      };
      auto SavedRegisters = [&]() {
        SmallVector<MCPhysReg, 8> Regs;
        for (const SavedPhysValue &Saved : ActiveSaved)
          Regs.push_back(Saved.Reg);
        return Regs;
      };
      auto UnitsForReg = [&](MCPhysReg Reg, AVMServiceValueKind Kind) {
        SmallVector<MCPhysReg, 2> Units;
        appendUnits(Units, Reg, Kind, TRI);
        return Units;
      };

      for (auto It = MBB.begin(); It != MBB.end();) {
        MachineInstr &MI = *It++;
        const AVMSystemServiceInfo *Info =
            getAVMSystemServiceInfo(MI.getOpcode());
        if (!Info) {
          const bool RegionBarrier = isRegionBarrier(MI);
          const bool StackBoundary = touchesArchitecturalStack(MI, TRI);

          if (!ActiveSaved.empty() &&
              (RegionBarrier || StackBoundary ||
              aliasesAnySaved(MI, ActiveSaved, TRI)))
            RestoreSaved(MI.getIterator(), MI.getDebugLoc());

          // No architectural PUSH16 save may remain active while an instruction
          // observes or changes the architectural stack pointer.
          assert(ActiveSaved.empty() || !StackBoundary);

          bool InvalidateAll = RegionBarrier;
          for (MCPhysReg Reg : {AVM::R0, AVM::R1, AVM::R2, AVM::R3, AVM::R4,
                                AVM::R5, AVM::R6, AVM::R7})
            if (InvalidateAll || MI.modifiesRegister(Reg, &TRI))
              Tokens[TokenIndex(Reg)] = NextToken++;
          continue;
        }

        uint8_t LiveMask = LiveAfter.lookup(&MI);
        SmallVector<PhysAssignment, 8> InputAssignments;
        SmallVector<PhysAssignment, 4> OutputAssignments;
        SmallVector<MCPhysReg, 8> Overwritten;

        unsigned OperandNo = Info->Outputs.size();
        bool ReadsSavedValue = false;
        for (const AVMServiceInputInfo &Input : Info->Inputs) {
          if (!Input.PassToMachine)
            continue;
          MachineOperand &SrcMO = MI.getOperand(OperandNo++);
          SmallVector<MCPhysReg, 2> SrcUnits =
              UnitsForReg(SrcMO.getReg().asMCReg(), Input.Kind);
          ReadsSavedValue |= llvm::any_of(SrcUnits, [&](MCPhysReg Unit) {
            return llvm::any_of(ActiveSaved, [&](const SavedPhysValue &Saved) {
              return TRI.regsOverlap(Unit, Saved.Reg);
            });
          });
        }
        bool ReplacesSavedValue = llvm::any_of(
            Info->Outputs, [&](const AVMServiceOutputInfo &Output) {
              return llvm::any_of(
                  ActiveSaved, [&](const SavedPhysValue &Saved) {
                    return TRI.regsOverlap(Output.PhysReg, Saved.Reg);
                  });
            });
        if (ReadsSavedValue || ReplacesSavedValue)
          RestoreSaved(MI.getIterator(), MI.getDebugLoc());

        for (const AVMServiceOutputInfo &Output : Info->Outputs) {
          MachineOperand &DestMO = MI.getOperand(Output.ResultIndex);
          assert(DestMO.isReg() && DestMO.getReg().isPhysical() &&
                 "service expansion must run after register allocation");
          appendAssignmentUnits(OutputAssignments, DestMO.getReg().asMCReg(),
                                Output.PhysReg, Output.Kind, TRI);
          appendUnits(Overwritten, Output.PhysReg, Output.Kind, TRI);
        }

        OperandNo = Info->Outputs.size();
        for (const AVMServiceInputInfo &Input : Info->Inputs) {
          if (!Input.PassToMachine)
            continue;
          MachineOperand &SrcMO = MI.getOperand(OperandNo++);
          assert(SrcMO.isReg() && SrcMO.getReg().isPhysical() &&
                 "service expansion must run after register allocation");
          appendAssignmentUnits(InputAssignments, Input.PhysReg,
                                SrcMO.getReg().asMCReg(), Input.Kind, TRI);
          SmallVector<MCPhysReg, 2> DestUnits;
          SmallVector<MCPhysReg, 2> SrcUnits;
          appendUnits(DestUnits, Input.PhysReg, Input.Kind, TRI);
          appendUnits(SrcUnits, SrcMO.getReg().asMCReg(), Input.Kind, TRI);
          for (unsigned I = 0; I != DestUnits.size(); ++I)
            if (Tokens[TokenIndex(DestUnits[I])] !=
                Tokens[TokenIndex(SrcUnits[I])])
              Overwritten.push_back(DestUnits[I]);
        }
        llvm::sort(Overwritten);
        Overwritten.erase(std::unique(Overwritten.begin(), Overwritten.end()),
                          Overwritten.end());

        SmallVector<MCPhysReg, 8> Saved;
        for (MCPhysReg Reg : Overwritten) {
          bool IsLive = LiveMask & (1u << TRI.getEncodingValue(Reg));
          if (!IsLive || MI.definesRegister(Reg, &TRI) ||
              llvm::any_of(ActiveSaved, [&](const SavedPhysValue &Value) {
                return Value.Reg == Reg;
              }))
            continue;
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::PUSH16)).addReg(Reg);
          ActiveSaved.push_back({Reg, Tokens[TokenIndex(Reg)]});
        }

        SmallVector<uint64_t, 8> InputTokens;
        SmallVector<PhysAssignment, 8> ResolvedInputs = InputAssignments;
        for (auto [I, Assignment] : llvm::enumerate(InputAssignments)) {
          uint64_t Token = Tokens[TokenIndex(Assignment.Src)];
          InputTokens.push_back(Token);
          if (Tokens[TokenIndex(Assignment.Dest)] == Token)
            ResolvedInputs[I].Src = ResolvedInputs[I].Dest;
        }
        Saved = SavedRegisters();
        ParallelCopyResolver(MF, TII, MBB, MI, LiveMask, Saved)
            .emit(std::move(ResolvedInputs));
        for (auto [Assignment, Token] :
             llvm::zip_equal(InputAssignments, InputTokens))
          Tokens[TokenIndex(Assignment.Dest)] = Token;

        MachineInstrBuilder Sys =
            BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(MI.getOpcode()));
        for (const AVMServiceOutputInfo &Output : Info->Outputs) {
          Sys.addReg(Output.PhysReg, RegState::Define);
          for (MCPhysReg Unit : UnitsForReg(Output.PhysReg, Output.Kind))
            Tokens[TokenIndex(Unit)] = NextToken++;
        }
        for (const AVMServiceInputInfo &Input : Info->Inputs)
          if (Input.PassToMachine)
            Sys.addReg(Input.PhysReg);
        Sys.cloneMemRefs(MI);

        // Results must leave their ABI registers before an incoming
        // live-through value is restored over those registers.
        SmallVector<uint64_t, 4> OutputTokens;
        SmallVector<PhysAssignment, 4> ResolvedOutputs = OutputAssignments;
        for (auto [I, Assignment] : llvm::enumerate(OutputAssignments)) {
          uint64_t Token = Tokens[TokenIndex(Assignment.Src)];
          OutputTokens.push_back(Token);
          if (Tokens[TokenIndex(Assignment.Dest)] == Token)
            ResolvedOutputs[I].Src = ResolvedOutputs[I].Dest;
        }
        Saved = SavedRegisters();
        ParallelCopyResolver(MF, TII, MBB, MI, LiveMask, Saved)
            .emit(std::move(ResolvedOutputs));
        for (auto [Assignment, Token] :
             llvm::zip_equal(OutputAssignments, OutputTokens))
          Tokens[TokenIndex(Assignment.Dest)] = Token;

        MI.eraseFromParent();
        Changed = true;
      }
      if (!ActiveSaved.empty())
        RestoreSaved(MBB.end(), DebugLoc());
    }
    return Changed;
  }
};

} // namespace

char AVMExpandSystemServices::ID = 0;

INITIALIZE_PASS(AVMExpandSystemServices, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMExpandSystemServicesPass() {
  return new AVMExpandSystemServices();
}
