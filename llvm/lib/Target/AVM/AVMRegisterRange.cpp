//===-- AVMRegisterRange.cpp - Allocated register ranges --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVMRegisterRange.h"
#include "AVM.h"
#include "AVMInstrInfo.h"
#include "AVMSystemServiceInfo.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/Support/MathExtras.h"
#include <algorithm>
#include <iterator>

using namespace llvm;

namespace {
constexpr MCPhysReg Words[] = {AVM::R0, AVM::R1, AVM::R2, AVM::R3,
                               AVM::R4, AVM::R5, AVM::R6, AVM::R7};
using Interval = AVMRegisterRange::Interval;

Interval add(Interval A, Interval B) {
  unsigned Lo = A.Lo + B.Lo, Hi = A.Hi + B.Hi;
  if ((Lo >> 16) != (Hi >> 16))
    return {};
  return {Lo & UINT16_MAX, Hi & UINT16_MAX};
}

Interval subtract(Interval A, Interval B) {
  if (A.Lo < B.Hi && A.Hi >= B.Lo)
    return {};
  return {uint16_t(A.Lo - B.Hi), uint16_t(A.Hi - B.Lo)};
}

bool byteResult(unsigned Opcode) {
  switch (Opcode) {
  case AVM::LD8U:
  case AVM::F5LD8U:
  case AVM::DPLD8U:
  case AVM::LDM8U:
  case AVM::LDSP8U:
  case AVM::LDSP8U_COMPACT:
  case AVM::LDP8U:
  case AVM::LDP8U_POST:
  case AVM::F7LD8U_POST:
  case AVM::GPLD8U_POST:
  case AVM::MUL8:
  case AVM::BOOL:
  case AVM::CSET_EQ:
  case AVM::CSET_NE:
  case AVM::CSET_ULT:
  case AVM::CSET_UGE:
  case AVM::CSET_SLT:
  case AVM::CSET_SGE:
    return true;
  default:
    return false;
  }
}
} // namespace

AVMRegisterRange::State::State(unsigned Size, bool IsReachable)
    : Reachable(IsReachable), Values(Size), Equal(Size) {
  for (unsigned I = 0; I != Size; ++I)
    Equal[I] = uint64_t(1) << I;
}

unsigned AVMRegisterRange::index(Register R) const {
  for (unsigned I = 0; I != 8; ++I)
    if (R == Words[I])
      return I;
  return Size;
}

AVMRegisterRange::Interval AVMRegisterRange::get(const State &S,
                                                 Register R) const {
  unsigned I = index(R);
  return S.Reachable && I < 8 ? S.Values[I] : Interval{};
}

std::optional<std::pair<int64_t, uint64_t>>
AVMRegisterRange::privateAccess(const MachineInstr &MI) const {
  if (MI.memoperands().size() != 1 || MI.hasOrderedMemoryRef())
    return std::nullopt;
  const MachineMemOperand &MMO = **MI.memoperands_begin();
  const auto *PSV =
      dyn_cast_or_null<FixedStackPseudoSourceValue>(MMO.getPseudoValue());
  const MachineFrameInfo &Frame = MF.getFrameInfo();
  if (!PSV || !Frame.isSpillSlotObjectIndex(PSV->getFrameIndex()) ||
      Frame.isDeadObjectIndex(PSV->getFrameIndex()) || PSV->isAliased(&Frame) ||
      !MMO.getSize().hasValue())
    return std::nullopt;
  return {{Frame.getObjectOffset(PSV->getFrameIndex()) + MMO.getOffset(),
           MMO.getSize().getValue()}};
}

void AVMRegisterRange::step(State &S, const MachineInstr &MI) const {
  if (!S.Reachable || MI.isDebugInstr() || MI.getOpcode() == AVM::STACKCALL)
    return;
  const auto &TRI = TII.getRegisterInfo();
  unsigned Opcode = MI.getOpcode();
  struct Fact {
    Interval Value;
    uint64_t Equal = 0;
  };
  auto Source = [&](unsigned Op) -> Fact {
    if (Op >= MI.getNumOperands() || !MI.getOperand(Op).isReg() ||
        MI.getOperand(Op).isUndef())
      return {};
    unsigned I = index(MI.getOperand(Op).getReg());
    return I < 8 ? Fact{S.Values[I], S.Equal[I]} : Fact{};
  };
  SmallVector<std::pair<unsigned, Fact>, 2> Results;
  unsigned Dest = MI.getNumOperands() && MI.getOperand(0).isReg()
                      ? index(MI.getOperand(0).getReg())
                      : Size;
  if ((Opcode == AVM::MOV || Opcode == AVM::MOV_RR) && Dest < 8) {
    Results.push_back({Dest, Source(1)});
  } else if (Opcode == AVM::MOV32 || Opcode == AVM::MOV32_F2) {
    Register D = MI.getOperand(0).getReg(), R = MI.getOperand(1).getReg();
    for (unsigned Sub : {AVM::sub_lo16, AVM::sub_hi16}) {
      unsigned DI = index(TRI.getSubReg(D, Sub));
      unsigned RI = index(TRI.getSubReg(R, Sub));
      if (DI < 8)
        Results.push_back({DI, RI < 8 && !MI.getOperand(1).isUndef()
                                   ? Fact{S.Values[RI], S.Equal[RI]}
                                   : Fact{}});
    }
  } else if (Dest < 8) {
    Fact V;
    bool Handled = true;
    if ((Opcode == AVM::LDI8 || Opcode == AVM::COLDLDI8 ||
         Opcode == AVM::LDI16 || Opcode == AVM::COLDLDI16) &&
        MI.getOperand(1).isImm()) {
      unsigned C = uint16_t(MI.getOperand(1).getImm());
      if (Opcode == AVM::LDI8 || Opcode == AVM::COLDLDI8)
        C &= 255;
      V.Value = {C, C};
    } else if (Opcode == AVM::ZEXT8) {
      Fact Input = Source(1);
      if (Input.Value.Hi <= 255)
        V = Input;
      else if ((Input.Value.Lo >> 8) == (Input.Value.Hi >> 8))
        V.Value = {Input.Value.Lo & 255, Input.Value.Hi & 255};
      else
        V.Value = {0, 255};
    } else if (Opcode == AVM::INC16 || Opcode == AVM::DEC16 ||
               Opcode == AVM::ADDIS8 || Opcode == AVM::COLDADDIS8) {
      int Step = Opcode == AVM::INC16 ? 1 : -1;
      if (Opcode == AVM::ADDIS8 || Opcode == AVM::COLDADDIS8)
        Step = int8_t(MI.getOperand(2).getImm());
      unsigned C = uint16_t(Step);
      V.Value = add(Source(1).Value, {C, C});
    } else if (Opcode == AVM::ADD || Opcode == AVM::ADD_RR) {
      V.Value = add(Source(1).Value, Source(2).Value);
    } else if (Opcode == AVM::SUB || Opcode == AVM::SUB_RR ||
               Opcode == AVM::XOR || Opcode == AVM::XOR_RR ||
               Opcode == AVM::AND || Opcode == AVM::AND_RR ||
               Opcode == AVM::OR || Opcode == AVM::OR_RR) {
      Fact A = Source(1), B = Source(2);
      // XOR/SUB of the same physical register is a real zero idiom, even
      // when the previous value is marked undef by register allocation.
      bool Same = MI.getOperand(1).getReg() == MI.getOperand(2).getReg() ||
                  (A.Equal & B.Equal);
      if (Opcode == AVM::SUB || Opcode == AVM::SUB_RR)
        V.Value = Same ? Interval{0, 0} : subtract(A.Value, B.Value);
      else if (Opcode == AVM::XOR || Opcode == AVM::XOR_RR)
        V.Value = Same
                      ? Interval{0, 0}
                      : Interval{0, unsigned(maskTrailingOnes<uint16_t>(
                                        Log2_32(A.Value.Hi | B.Value.Hi) + 1))};
      else if (Opcode == AVM::AND || Opcode == AVM::AND_RR) {
        if (Same)
          V = A;
        else if (A.Value.Lo == UINT16_MAX)
          V = B;
        else if (B.Value.Lo == UINT16_MAX)
          V = A;
        else
          V.Value = {0, std::min(A.Value.Hi, B.Value.Hi)};
      } else
        V.Value = {0, unsigned(maskTrailingOnes<uint16_t>(
                          Log2_32(A.Value.Hi | B.Value.Hi) + 1))};
    } else if (byteResult(Opcode))
      V.Value = {0, 255};
    else if (Opcode == AVM::LDSP16 || Opcode == AVM::LDSP16_COMPACT) {
      if (auto Access = privateAccess(MI); Access && Access->second == 2)
        if (auto It = Slots.find(Access->first); It != Slots.end())
          V = {S.Values[It->second], S.Equal[It->second]};
    } else
      Handled = false;
    if (Handled)
      Results.push_back({Dest, V});
  }

  std::optional<Fact> Store;
  auto Access = privateAccess(MI);
  if (Access && Access->second == 2 &&
      (Opcode == AVM::STSP16 || Opcode == AVM::STSP16_COMPACT))
    Store = Source(1);

  uint64_t Killed = 0;
  bool Opaque = MI.isBundled() || MI.isInlineAsm() ||
                (MI.isCall() && !isAVMSystemService(Opcode) &&
                 llvm::none_of(MI.operands(),
                               [](const MachineOperand &MO) {
                                 return MO.isRegMask();
                               })) ||
                (MI.hasUnmodeledSideEffects() && !isAVMSystemService(Opcode) &&
                 !MI.isCall() && !MI.isMetaInstruction());
  if (Opaque) {
    Results.clear();
    Store.reset();
  }
  for (unsigned I = 0; I != 8; ++I)
    if (Opaque || MI.modifiesRegister(Words[I], &TRI))
      Killed |= uint64_t(1) << I;
  if (MI.mayStore() && Access)
    for (auto [Offset, I] : Slots)
      if (Offset < Access->first + int64_t(Access->second) &&
          Access->first < Offset + 2)
        Killed |= uint64_t(1) << I;
  // Ordinary IR accesses and ABI calls cannot alias private spill objects.
  // An opaque instruction or unannotated stack write cannot supply that proof.
  if (Opaque ||
      (MI.mayStore() && !Access && !MI.isCall() &&
       !isAVMSystemService(Opcode) &&
       (MI.memoperands_empty() ||
        llvm::any_of(MI.memoperands(), [](const MachineMemOperand *M) {
          return !M->getValue();
        }))))
    for (unsigned I = 8; I != Size; ++I)
      Killed |= uint64_t(1) << I;

  if (MI.modifiesRegister(AVM::CC, &TRI) || Opaque || MI.isCall())
    S.CC.Valid = false;
  S.CC.Equal &= ~Killed;
  for (unsigned I = 0; I != Size; ++I) {
    S.Equal[I] &= ~Killed;
    if (Killed & (uint64_t(1) << I)) {
      S.Values[I] = {};
      S.Equal[I] = uint64_t(1) << I;
    }
  }
  auto Assign = [&](unsigned I, Fact V) {
    uint64_t Equal = (V.Equal & ~Killed) | (uint64_t(1) << I);
    S.Values[I] = V.Value;
    S.Equal[I] = Equal;
    for (unsigned J = 0; J != Size; ++J)
      if (Equal & (uint64_t(1) << J))
        S.Equal[J] = Equal;
    if (Equal & S.CC.Equal)
      S.CC.Equal |= uint64_t(1) << I;
  };
  for (auto [I, V] : Results)
    Assign(I, V);
  if (Store)
    if (auto It = Slots.find(Access->first); It != Slots.end())
      Assign(It->second, *Store);

  if (Opaque)
    return;
  if ((Opcode == AVM::CMPIS8 || Opcode == AVM::COLDCMPIS8) &&
      MI.getOperand(1).isImm()) {
    unsigned I = index(MI.getOperand(0).getReg());
    if (I < 8 && !MI.getOperand(0).isUndef())
      S.CC = {true, S.Equal[I], S.Values[I],
              uint16_t(int8_t(MI.getOperand(1).getImm()))};
  } else if (Opcode == AVM::CMP || Opcode == AVM::CMP_RR) {
    unsigned L = index(MI.getOperand(0).getReg());
    unsigned R = index(MI.getOperand(1).getReg());
    if (L < 8 && R < 8 && !MI.getOperand(0).isUndef() &&
        !MI.getOperand(1).isUndef()) {
      if (S.Values[R].Lo == S.Values[R].Hi)
        S.CC = {true, S.Equal[L], S.Values[L], S.Values[R].Lo};
      else if (S.Values[L].Lo == S.Values[L].Hi)
        S.CC = {true, S.Equal[R], S.Values[R], S.Values[L].Lo, true};
    }
  } else if (Opcode == AVM::TST16 || Opcode == AVM::TST8) {
    unsigned I = index(MI.getOperand(0).getReg());
    if (I < 8 && !MI.getOperand(0).isUndef() &&
        (Opcode == AVM::TST16 || S.Values[I].Hi <= 255))
      S.CC = {true, S.Equal[I], S.Values[I], 0};
  }
}

void AVMRegisterRange::refine(State &S, unsigned Condition) const {
  if (!S.CC.Valid)
    return;
  unsigned Lo = S.CC.Value.Lo, Hi = S.CC.Value.Hi, C = S.CC.Constant;
  switch (Condition) {
  case AVMCC::EQ:
    Lo = std::max(Lo, C);
    Hi = std::min(Hi, C);
    break;
  case AVMCC::NE:
    if (Lo == C)
      ++Lo;
    if (Hi == C) {
      if (!Hi) {
        S.Reachable = false;
        return;
      }
      --Hi;
    }
    break;
  case AVMCC::ULT:
    if (S.CC.Reversed) {
      Lo = std::max(Lo, C + 1);
      break;
    }
    if (!C) {
      S.Reachable = false;
      return;
    }
    Hi = std::min(Hi, C - 1);
    break;
  case AVMCC::UGE:
    if (S.CC.Reversed)
      Hi = std::min(Hi, C);
    else
      Lo = std::max(Lo, C);
    break;
  default:
    return;
  }
  if (Lo > Hi) {
    S.Reachable = false;
    return;
  }
  for (unsigned I = 0; I != Size; ++I)
    if (S.CC.Equal & (uint64_t(1) << I)) {
      S.Values[I] = {std::max(S.Values[I].Lo, Lo),
                     std::min(S.Values[I].Hi, Hi)};
      if (S.Values[I].Lo > S.Values[I].Hi)
        S.Reachable = false;
    }
}

bool AVMRegisterRange::join(State &Dest, const State &Source) const {
  if (!Source.Reachable)
    return false;
  if (!Dest.Reachable) {
    Dest = Source;
    Dest.CC.Valid = false;
    return true;
  }
  bool Changed = false;
  for (unsigned I = 0; I != Size; ++I) {
    Interval R{std::min(Dest.Values[I].Lo, Source.Values[I].Lo),
               std::max(Dest.Values[I].Hi, Source.Values[I].Hi)};
    // Widen to comparison boundaries instead of enumerating every iteration.
    // For i = 0; ++i != 32, [1,31] is stable on the backedge. If the word
    // could reach 256, ZEXT8 stops preserving identity and cannot constrain i.
    if (R.Lo < Dest.Values[I].Lo)
      R.Lo = *std::prev(llvm::upper_bound(Thresholds, R.Lo));
    if (R.Hi > Dest.Values[I].Hi)
      R.Hi = *llvm::lower_bound(Thresholds, R.Hi);
    uint64_t Equal = Dest.Equal[I] & Source.Equal[I];
    Changed |= !(R == Dest.Values[I]) || Equal != Dest.Equal[I];
    Dest.Values[I] = R;
    Dest.Equal[I] = Equal;
  }
  Dest.CC.Valid = false;
  return Changed;
}

AVMRegisterRange::AVMRegisterRange(MachineFunction &MF, const AVMInstrInfo &TII)
    : MF(MF), TII(TII) {
  Thresholds = {0, 1, 255, 256, UINT16_MAX};
  for (MachineBasicBlock &MBB : MF)
    for (const MachineInstr &MI : MBB) {
      // Final frame offsets account for distinct spill objects sharing storage
      // after stack-slot coloring. Partial and overlapping stores kill facts.
      if (auto Access = privateAccess(MI); Access && Access->second == 2 &&
                                           !Slots.contains(Access->first) &&
                                           Size < 64)
        Slots[Access->first] = Size++;
      bool Compare =
          MI.getOpcode() == AVM::CMPIS8 || MI.getOpcode() == AVM::COLDCMPIS8;
      bool Constant =
          MI.getOpcode() == AVM::LDI8 || MI.getOpcode() == AVM::COLDLDI8 ||
          MI.getOpcode() == AVM::LDI16 || MI.getOpcode() == AVM::COLDLDI16;
      if ((Compare || Constant) && MI.getOperand(1).isImm()) {
        unsigned C = Compare ? uint16_t(int8_t(MI.getOperand(1).getImm()))
                             : uint16_t(MI.getOperand(1).getImm());
        if (Constant && C > 256)
          continue;
        Thresholds.push_back(C);
        if (C)
          Thresholds.push_back(C - 1);
        if (C < UINT16_MAX)
          Thresholds.push_back(C + 1);
      }
    }
  llvm::sort(Thresholds);
  Thresholds.erase(llvm::unique(Thresholds), Thresholds.end());
  if (MF.empty())
    return;
  Entries.try_emplace(&MF.front(), Size, true);
  SmallVector<MachineBasicBlock *, 16> Worklist{&MF.front()};
  SmallPtrSet<MachineBasicBlock *, 16> Queued{&MF.front()};
  unsigned Steps = 0;
  while (!Worklist.empty()) {
    if (++Steps > 8192) {
      Entries.clear(); // No partial fixed point may justify a rewrite.
      return;
    }
    MachineBasicBlock *MBB = Worklist.pop_back_val();
    Queued.erase(MBB);
    State Out = Entries.find(MBB)->second;
    for (const MachineInstr &MI : *MBB)
      step(Out, MI);
    MachineBasicBlock *TBB = nullptr, *FBB = nullptr;
    SmallVector<MachineOperand, 2> Cond;
    bool Refine = !TII.analyzeBranch(*MBB, TBB, FBB, Cond, false) &&
                  MBB->succ_size() == 2 && TBB && Cond.size() == 1 &&
                  Cond[0].isImm();
    for (MachineBasicBlock *Succ : MBB->successors()) {
      State Edge = Out;
      if (Refine)
        refine(Edge, unsigned(Cond[0].getImm()) ^ unsigned(Succ != TBB));
      auto [It, Inserted] = Entries.try_emplace(Succ, Size, false);
      if (join(It->second, Edge) && Queued.insert(Succ).second)
        Worklist.push_back(Succ);
    }
  }
}

AVMRegisterRange::State
AVMRegisterRange::getEntry(const MachineBasicBlock &MBB) const {
  auto It = Entries.find(&MBB);
  return It == Entries.end() ? State(Size, true) : It->second;
}
