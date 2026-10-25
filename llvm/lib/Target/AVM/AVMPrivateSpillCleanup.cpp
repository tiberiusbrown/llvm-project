//===-- AVMPrivateSpillCleanup.cpp - Optimize allocated spill slots ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "AVMSystemServiceInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/LiveStacks.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/SlotIndexes.h"
#include "llvm/InitializePasses.h"
#include <optional>

using namespace llvm;

namespace {
struct StackAccess {
  int FI;
  uint64_t Offset;
  unsigned Size;
};

static std::optional<StackAccess> stackAccess(const MachineInstr &MI) {
  unsigned Size;
  switch (MI.getOpcode()) {
  case AVM::STACK_LOAD8U_PSEUDO:
  case AVM::STACK_LOAD8S_PSEUDO:
  case AVM::STACK_STORE8_PSEUDO:
    Size = 1;
    break;
  case AVM::STACK_LOAD16_PSEUDO:
  case AVM::STACK_STORE16_PSEUDO:
    Size = 2;
    break;
  case AVM::STACK_LOAD24_PSEUDO:
  case AVM::STACK_STORE24_PSEUDO:
    Size = 3;
    break;
  case AVM::STACK_LOAD32_PSEUDO:
  case AVM::STACK_STORE32_PSEUDO:
    Size = 4;
    break;
  default:
    return std::nullopt;
  }
  unsigned FIOp = MI.mayLoad() ? 1 : 0;
  if (!MI.getOperand(FIOp).isFI() ||
      !MI.getOperand(FIOp + 1).isImm() ||
      MI.getOperand(FIOp + 1).getImm() < 0)
    return std::nullopt;
  return StackAccess{MI.getOperand(FIOp).getIndex(),
                     uint64_t(MI.getOperand(FIOp + 1).getImm()), Size};
}

// ABI calls and ordinary IR memory accesses cannot observe private spills.
// Unannotated memory operations and opaque assembly do not supply that proof.
static bool isBarrier(const MachineInstr &MI) {
  if (MI.isBundled())
    return true;
  return !MI.isCall() && !MI.isReturn() &&
         !isAVMSystemService(MI.getOpcode()) &&
         !MI.isMetaInstruction() &&
         ((MI.hasUnmodeledSideEffects() && !MI.isTerminator()) ||
          (MI.mayLoadOrStore() &&
           (MI.memoperands_empty() ||
            any_of(MI.memoperands(), [](const MachineMemOperand *MMO) {
              return !MMO->getValue() && !MMO->getPseudoValue();
            }))));
}

class AVMPrivateSpillCleanup final : public MachineFunctionPass {
public:
  static char ID;
  AVMPrivateSpillCleanup() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return "AVM private spill cleanup"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addPreserved<SlotIndexesWrapperPass>();
    // Removing accesses only shortens stack lifetimes. The existing intervals
    // remain conservative for coloring, which ignores removed frame objects.
    AU.addPreserved<LiveStacksWrapperLegacy>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    MachineFrameInfo &MFI = MF.getFrameInfo();
    const auto &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
    const auto &TRI = TII.getRegisterInfo();
    auto *Wrapper = getAnalysisIfAvailable<SlotIndexesWrapperPass>();
    SlotIndexes *Indexes = Wrapper ? &Wrapper->getSI() : nullptr;

    DenseSet<int> Slots;
    for (int FI = 0, E = MFI.getObjectIndexEnd(); FI != E; ++FI)
      if (MFI.isSpillSlotObjectIndex(FI) && !MFI.isDeadObjectIndex(FI))
        Slots.insert(FI);
    // Exclude escaping addresses, ordered accesses and malformed ranges before
    // using the fact that different spill objects cannot alias one another.
    for (MachineBasicBlock &MBB : MF)
      for (MachineInstr &MI : MBB.instrs()) {
        auto Access = stackAccess(MI);
        for (const MachineOperand &MO : MI.operands())
          if (MO.isFI() && Slots.contains(MO.getIndex()) &&
              (!Access || MI.isBundled() || Access->FI != MO.getIndex() ||
               MI.hasOrderedMemoryRef() ||
               uint64_t(Access->Offset) + Access->Size >
                   uint64_t(MFI.getObjectSize(Access->FI))))
            Slots.erase(MO.getIndex());
        for (const MachineMemOperand *MMO : MI.memoperands())
          if (const auto *PSV = dyn_cast_or_null<FixedStackPseudoSourceValue>(
                  MMO->getPseudoValue()))
            if (!Access || MI.isBundled() ||
                PSV->getFrameIndex() != Access->FI ||
                MMO->getOffset() != Access->Offset ||
                MMO->getSize() != LocationSize::precise(Access->Size) ||
                MI.hasOrderedMemoryRef())
              Slots.erase(PSV->getFrameIndex());
      }
    if (Slots.empty())
      return false;

    auto Erase = [&](MachineInstr &MI) {
      if (Indexes)
        Indexes->removeMachineInstrFromMaps(MI);
      MI.eraseFromParent();
    };
    bool Changed = false;
    DenseSet<MCRegister> Extended;
    for (MachineBasicBlock &MBB : MF) {
      struct SavedValue {
        uint64_t Offset;
        unsigned Size;
        unsigned Value;
      };
      DenseMap<int, SavedValue> Available;
      // Value identities retain all copies of a saved value, including copies
      // made before the store. Overwriting one carrier must not hide another.
      DenseMap<Register, unsigned> Values;
      unsigned NextValue = 0;
      for (MCPhysReg Reg : AVM::GPR16RegClass)
        Values[Reg] = ++NextValue;
      for (MCPhysReg Reg : AVM::GPR32RegClass)
        Values[Reg] = ++NextValue;
      for (MachineInstr &Original : make_early_inc_range(MBB)) {
        if (Original.isDebugInstr())
          continue;
        MachineInstr *MI = &Original;
        auto Access = stackAccess(*MI);
        bool Private = Access && Slots.contains(Access->FI);
        if (!Private && isBarrier(*MI))
          Available.clear();

        SmallVector<std::pair<Register, unsigned>, 3> Results;
        if (Private && MI->mayLoad() &&
            (Access->Size == 2 || Access->Size == 4)) {
          auto It = Available.find(Access->FI);
          bool Known = It != Available.end() &&
                       It->second.Offset == Access->Offset &&
                       It->second.Size == Access->Size;
          Register Dst = MI->getOperand(0).getReg(), Src;
          unsigned Value = Known ? It->second.Value : ++NextValue;
          const auto &RC = Access->Size == 2 ? AVM::GPR16RegClass
                                           : AVM::GPR32RegClass;
          if (Values.lookup(Dst) == Value)
            Src = Dst;
          else if (Known)
            for (MCPhysReg Reg : RC)
              if (Values.lookup(Reg) == Value) {
                Src = Reg;
                break;
              }
          if (Src) {
            // Forwarding extends this physical register's use past any earlier
            // kill flags, including uses of its overlapping register pair.
            for (MCRegAliasIterator Alias(Src, &TRI, true); Alias.isValid();
                 ++Alias)
              Extended.insert(*Alias);
            if (Src == Dst) {
              Erase(*MI);
              Changed = true;
              continue;
            }
            auto Copy = BuildMI(MBB, *MI, MI->getDebugLoc(),
                                TII.get(TargetOpcode::COPY), Dst)
                            .addReg(Src);
            if (Indexes)
              Indexes->replaceMachineInstrInMaps(*MI, *Copy);
            MI->eraseFromParent();
            MI = Copy;
            Changed = true;
          } else {
            Results.emplace_back(Dst, Value);
            Available[Access->FI] = {Access->Offset, Access->Size, Value};
          }
        }

        if ((MI->isCopy() || MI->getOpcode() == AVM::COPY16_PSEUDO ||
             MI->getOpcode() == AVM::COPY32_PSEUDO) &&
            !MI->getOperand(1).isUndef() &&
            !MI->getOperand(0).getSubReg() && !MI->getOperand(1).getSubReg()) {
          Register Dst = MI->getOperand(0).getReg();
          Register Src = MI->getOperand(1).getReg();
          if (Values.count(Dst) && Values.count(Src)) {
            Results.emplace_back(Dst, Values.lookup(Src));
            if (AVM::GPR32RegClass.contains(Dst, Src))
              for (unsigned Sub : {AVM::sub_lo16, AVM::sub_hi16})
                Results.emplace_back(TRI.getSubReg(Dst, Sub),
                                     Values.lookup(TRI.getSubReg(Src, Sub)));
          }
        }
        if (Private && MI->mayStore()) {
          Available.erase(Access->FI);
          if ((Access->Size == 2 || Access->Size == 4) &&
              !MI->getOperand(2).isUndef())
            Available[Access->FI] = {Access->Offset, Access->Size,
                                    Values.lookup(MI->getOperand(2).getReg())};
        }
        bool UnknownClobbers =
            MI->isBundled() ||
            (MI->isCall() && !isAVMSystemService(MI->getOpcode()) &&
             none_of(MI->operands(), [](const MachineOperand &MO) {
               return MO.isRegMask();
             }));
        for (auto &Entry : Values)
          if (UnknownClobbers || MI->modifiesRegister(Entry.first, &TRI))
            Entry.second = ++NextValue;
        for (auto [Reg, Value] : Results)
          Values[Reg] = Value;
      }
    }

    for (MCRegister Reg : Extended)
      MF.getRegInfo().clearKillFlags(Reg);

    // Forwarding may remove the last reader of a saved value. Compute byte
    // liveness over the CFG, independently of physical register liveness, so
    // partial overlaps, successor reads and loop-carried spills are respected.
    DenseMap<int, unsigned> SlotStart;
    unsigned NumBytes = 0;
    for (int FI : Slots) {
      SlotStart[FI] = NumBytes;
      NumBytes += MFI.getObjectSize(FI);
    }
    struct BlockLiveness {
      SmallBitVector Use, Def, In;
    };
    DenseMap<const MachineBasicBlock *, BlockLiveness> Liveness;
    for (MachineBasicBlock &MBB : MF) {
      SmallBitVector Use(NumBytes), Def(NumBytes);
      for (MachineInstr &MI : MBB) {
        if (MI.isDebugInstr())
          continue;
        auto Access = stackAccess(MI);
        if (Access && Slots.contains(Access->FI)) {
          unsigned Start = SlotStart.lookup(Access->FI) + Access->Offset;
          for (unsigned I = Start, E = Start + Access->Size; I != E; ++I)
            if (MI.mayStore())
              Def.set(I);
            else if (!Def.test(I))
              Use.set(I);
        } else if (isBarrier(MI)) {
          SmallBitVector Unwritten = Def;
          Unwritten.flip();
          Use |= Unwritten;
        }
      }
      Liveness[&MBB] = {
          std::move(Use), std::move(Def), SmallBitVector(NumBytes)};
    }
    bool Again;
    do {
      Again = false;
      for (MachineBasicBlock &MBB : reverse(MF)) {
        auto &Live = Liveness[&MBB];
        SmallBitVector In(NumBytes);
        for (const MachineBasicBlock *Succ : MBB.successors())
          In |= Liveness[Succ].In;
        In.reset(Live.Def);
        In |= Live.Use;
        if (In != Live.In) {
          Live.In = std::move(In);
          Again = true;
        }
      }
    } while (Again);

    for (MachineBasicBlock &MBB : MF) {
      SmallBitVector Live(NumBytes);
      for (const MachineBasicBlock *Succ : MBB.successors())
        Live |= Liveness[Succ].In;
      for (MachineInstr &MI : make_early_inc_range(reverse(MBB))) {
        if (MI.isDebugInstr())
          continue;
        auto Access = stackAccess(MI);
        if (!Access || !Slots.contains(Access->FI)) {
          if (isBarrier(MI))
            Live.set();
          continue;
        }
        unsigned Start = SlotStart.lookup(Access->FI) + Access->Offset;
        unsigned End = Start + Access->Size;
        if (MI.mayLoad()) {
          Live.set(Start, End);
        } else {
          bool Needed = false;
          for (unsigned I = Start; I != End; ++I)
            Needed |= Live.test(I);
          if (!Needed) {
            Erase(MI);
            Changed = true;
          }
          Live.reset(Start, End);
        }
      }
    }

    // Retain objects referenced by instructions or debug information.
    for (MachineBasicBlock &MBB : MF)
      for (MachineInstr &MI : MBB.instrs())
        for (const MachineOperand &MO : MI.operands())
          if (MO.isFI())
            Slots.erase(MO.getIndex());
    for (int FI : Slots) {
      MFI.RemoveStackObject(FI);
      Changed = true;
    }
    return Changed;
  }
};
} // namespace

char AVMPrivateSpillCleanup::ID = 0;
INITIALIZE_PASS(AVMPrivateSpillCleanup, "avm-private-spill-cleanup",
                "AVM private spill cleanup", false, false)

FunctionPass *llvm::createAVMPrivateSpillCleanupPass() {
  return new AVMPrivateSpillCleanup();
}
