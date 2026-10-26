//===-- AVMLatePeephole.cpp - Physical-register AVM cleanups ------------===//

#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMInstrInfo.h"
#include "AVMRegisterRange.h"
#include "AVMSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/InitializePasses.h"
#include "llvm/MC/MCDwarf.h"
#include <array>
#include <iterator>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "avm-late-peephole"
#define PASS_NAME "AVM late peephole"

namespace {

static bool isWordStackStore(const MachineInstr &MI) {
  return MI.getOpcode() == AVM::STSP16_COMPACT ||
         MI.getOpcode() == AVM::STSP16;
}

static bool isWordStackLoad(const MachineInstr &MI) {
  return MI.getOpcode() == AVM::LDSP16_COMPACT ||
         MI.getOpcode() == AVM::LDSP16;
}

static bool sameStackOffset(const MachineInstr &Store,
                            const MachineInstr &Other, bool IsLoad) {
  unsigned OtherOffset = IsLoad ? 1 : 0;
  return Store.getOperand(0).isImm() &&
         Other.getOperand(OtherOffset).isImm() &&
         Store.getOperand(0).getImm() ==
             Other.getOperand(OtherOffset).getImm();
}

static bool removeRedundantStackReloads(MachineBasicBlock &MBB,
                                        const AVMRegisterInfo &TRI) {
  bool Changed = false;
  for (auto It = MBB.begin(); It != MBB.end();) {
    auto Store = It++;
    if (!isWordStackStore(*Store) || Store->hasOrderedMemoryRef())
      continue;
    auto Load = It;
    while (Load != MBB.end() && Load->isDebugInstr())
      ++Load;
    if (Load == MBB.end() || !isWordStackLoad(*Load) ||
        Load->hasOrderedMemoryRef() || !sameStackOffset(*Store, *Load, true) ||
        Store->getOperand(1).getReg() != Load->getOperand(0).getReg())
      continue;

    // The reload recreates the same register value. The first store is also
    // dead if another store replaces it before any operation can observe it.
    bool Overwritten = false;
    for (auto Scan = std::next(Load); Scan != MBB.end(); ++Scan) {
      if (Scan->isDebugInstr())
        continue;
      if (isWordStackStore(*Scan) && !Scan->hasOrderedMemoryRef() &&
          sameStackOffset(*Store, *Scan, false)) {
        Overwritten = true;
        break;
      }
      if (Scan->mayLoadOrStore() || Scan->isCall() || Scan->isTerminator() ||
          Scan->hasUnmodeledSideEffects() ||
          Scan->modifiesRegister(AVM::SP, &TRI))
        break;
    }
    It = std::next(Load);
    MBB.erase(Load);
    if (Overwritten)
      MBB.erase(Store);
    else
      // The reload no longer defines the register used by later instructions.
      // Keep the saved value for other readers, but extend its register lifetime
      // past the store if register allocation marked that use as a kill.
      Store->getOperand(1).setIsKill(false);
    Changed = true;
  }
  return Changed;
}

static bool isWordCompare(const MachineInstr &MI) {
  return MI.getOpcode() == AVM::CMP || MI.getOpcode() == AVM::CMP_RR;
}

static bool producesZeroExtendedByte(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case AVM::LDI8:
  case AVM::COLDLDI8:
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
  case AVM::ZEXT8:
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

// Facts are local to a basic block. Identities describe values rather than
// mutable registers, and known bits describe the complete 16-bit word.
struct WordValue {
  unsigned Identity = 0;
  uint16_t Zero = 0, One = 0;

  bool isConstant() const { return uint16_t(Zero | One) == UINT16_MAX; }
  bool equals(const WordValue &Other) const {
    return (Identity && Identity == Other.Identity) ||
           (isConstant() && Other.isConstant() && One == Other.One);
  }
  static WordValue constant(uint16_t V) { return {0, uint16_t(~V), V}; }
};

static bool cleanRegisterAndStackValues(MachineBasicBlock &MBB,
                                        const AVMInstrInfo &TII,
                                        const AVMRegisterRange &Ranges) {
  const AVMRegisterInfo &TRI = TII.getRegisterInfo();
  constexpr MCPhysReg Words[] = {AVM::R0, AVM::R1, AVM::R2, AVM::R3,
                                 AVM::R4, AVM::R5, AVM::R6, AVM::R7};
  auto Index = [&](Register R) {
    for (unsigned I = 0; I != 8; ++I)
      if (R == Words[I])
        return I;
    return 8u;
  };
  unsigned NextIdentity = 0;
  auto Fresh = [&]() { return WordValue{++NextIdentity}; };
  std::array<WordValue, 8> Values;
  for (WordValue &V : Values)
    V = Fresh();
  DenseMap<int64_t, WordValue> Stack;
  unsigned ChangedRegisters = 0;
  auto ChangedReg = [&](Register R) {
    for (unsigned I = 0; I != 8; ++I)
      if (TRI.regsOverlap(R, Words[I]))
        ChangedRegisters |= 1u << I;
  };
  bool Changed = false;
  auto RangeState = Ranges.getEntry(MBB);
  for (auto It = MBB.begin(); It != MBB.end();) {
    MachineInstr &MI = *It++;
    bool ByteInput = MI.getOpcode() == AVM::ZEXT8 &&
                     !MI.getOperand(1).isUndef() &&
                     Ranges.get(RangeState, MI.getOperand(1).getReg()).Hi <= 255;
    Ranges.step(RangeState, MI);
    // STACKCALL is a zero-size annotation consumed by the assembly printer.
    // The following call, rather than its annotation, carries the clobbers.
    if (MI.isDebugInstr() || MI.getOpcode() == AVM::STACKCALL)
      continue;
    // IMPLICIT_DEF and other metadata can end a physical value's lifetime.
    // They emit no code, but must still invalidate facts about their defs.
    if (MI.isMetaInstruction()) {
      for (unsigned I = 0; I != 8; ++I)
        if (MI.modifiesRegister(Words[I], &TRI))
          Values[I] = Fresh();
      if (MI.modifiesRegister(AVM::SP, &TRI) || MI.mayStore() ||
          MI.hasUnmodeledSideEffects())
        Stack.clear();
      continue;
    }
    if (MI.isBundled()) {
      Stack.clear();
      for (WordValue &V : Values)
        V = Fresh();
      continue;
    }
    SmallVector<std::pair<unsigned, WordValue>, 2> Results;
    bool Remove = false;
    bool HandledStack = false;
    unsigned Dest = MI.getNumOperands() && MI.getOperand(0).isReg()
                        ? Index(MI.getOperand(0).getReg())
                        : 8;
    unsigned Opcode = MI.getOpcode();
    if ((Opcode == AVM::MOV || Opcode == AVM::MOV_RR) && Dest != 8) {
      unsigned Src = Index(MI.getOperand(1).getReg());
      if (Src != 8) {
        Remove = Values[Dest].equals(Values[Src]);
        Results.emplace_back(Dest, MI.getOperand(1).isUndef() ? Fresh()
                                                              : Values[Src]);
        if (MI.getOperand(1).isUndef())
          Remove = false;
      }
    } else if (Opcode == AVM::MOV32 || Opcode == AVM::MOV32_F2) {
      Register D = MI.getOperand(0).getReg(), S = MI.getOperand(1).getReg();
      unsigned DL = Index(TRI.getSubReg(D, AVM::sub_lo16));
      unsigned DH = Index(TRI.getSubReg(D, AVM::sub_hi16));
      unsigned SL = Index(TRI.getSubReg(S, AVM::sub_lo16));
      unsigned SH = Index(TRI.getSubReg(S, AVM::sub_hi16));
      if (DL != 8 && DH != 8 && SL != 8 && SH != 8 &&
          !MI.getOperand(1).isUndef()) {
        Remove = Values[DL].equals(Values[SL]) && Values[DH].equals(Values[SH]);
        Results.emplace_back(DL, Values[SL]);
        Results.emplace_back(DH, Values[SH]);
      }
    } else if (Dest != 8 &&
               (Opcode == AVM::LDI8 || Opcode == AVM::COLDLDI8 ||
                Opcode == AVM::LDI16 || Opcode == AVM::COLDLDI16) &&
               MI.getOperand(1).isImm()) {
      uint16_t V = MI.getOperand(1).getImm();
      if (Opcode == AVM::LDI8 || Opcode == AVM::COLDLDI8)
        V &= 255;
      Results.emplace_back(Dest, WordValue::constant(V));
      Remove = Values[Dest].equals(Results.back().second);
    } else if (Dest != 8 && Opcode == AVM::ZEXT8) {
      unsigned Src = Index(MI.getOperand(1).getReg());
      if (Src != 8) {
        WordValue V = MI.getOperand(1).isUndef() ? Fresh() : Values[Src];
        if (ByteInput)
          V.Zero |= 0xff00;
        if ((V.Zero & 0xff00) == 0xff00) {
          Remove = Values[Dest].equals(V);
          Results.emplace_back(Dest, V);
        } else {
          V.Identity = ++NextIdentity;
          V.Zero |= 0xff00;
          V.One &= 0xff;
          Results.emplace_back(Dest, V);
        }
      }
    } else if (Dest != 8 && (Opcode == AVM::XOR || Opcode == AVM::XOR_RR ||
                             Opcode == AVM::SUB || Opcode == AVM::SUB_RR ||
                             Opcode == AVM::AND || Opcode == AVM::AND_RR ||
                             Opcode == AVM::OR || Opcode == AVM::OR_RR)) {
      unsigned L = Index(MI.getOperand(1).getReg());
      unsigned R = Index(MI.getOperand(2).getReg());
      if (L != 8 && R != 8) {
        WordValue A = MI.getOperand(1).isUndef() ? Fresh() : Values[L];
        WordValue B = MI.getOperand(2).isUndef() ? Fresh() : Values[R];
        WordValue V = Fresh();
        if (Opcode == AVM::XOR || Opcode == AVM::XOR_RR) {
          V.Zero = (A.Zero & B.Zero) | (A.One & B.One);
          V.One = (A.Zero & B.One) | (A.One & B.Zero);
          if (L == R || A.equals(B))
            V = WordValue::constant(0);
        } else if (Opcode == AVM::AND || Opcode == AVM::AND_RR) {
          V.Zero = A.Zero | B.Zero;
          V.One = A.One & B.One;
          if (A.equals(B))
            V = A;
        } else if (Opcode == AVM::OR || Opcode == AVM::OR_RR) {
          V.Zero = A.Zero & B.Zero;
          V.One = A.One | B.One;
          if (A.equals(B))
            V = A;
        } else if (L == R || A.equals(B)) {
          V = WordValue::constant(0);
        }
        Remove = Values[Dest].equals(V);
        Results.emplace_back(Dest, V);
      }
    } else if (Dest != 8 && producesZeroExtendedByte(MI)) {
      WordValue V = Fresh();
      V.Zero = 0xff00;
      Results.emplace_back(Dest, V);
    } else if (Opcode == AVM::LDP24 || Opcode == AVM::LDP24_POST) {
      unsigned Hi =
          Index(TRI.getSubReg(MI.getOperand(0).getReg(), AVM::sub_hi16));
      if (Hi != 8) {
        WordValue V = Fresh();
        V.Zero = 0xff00;
        Results.emplace_back(Hi, V);
      }
    }

    if ((isWordStackLoad(MI) || isWordStackStore(MI)) &&
        !MI.hasOrderedMemoryRef()) {
      bool Load = isWordStackLoad(MI);
      unsigned OffsetOperand = Load ? 1 : 0;
      unsigned Reg = Load ? Dest : Index(MI.getOperand(1).getReg());
      const MachineOperand &Offset = MI.getOperand(OffsetOperand);
      if (Reg != 8 && Offset.isImm() && Offset.getImm() >= 0 &&
          Offset.getImm() < UINT16_MAX) {
        int64_t At = Offset.getImm();
        HandledStack = true;
        auto Found = Stack.find(At);
        if (Load) {
          WordValue V = Found == Stack.end() ? Fresh() : Found->second;
          Stack[At] = V;
          Remove = Values[Reg].equals(V);
          if (!Remove) {
            // A full-register MOV costs 37 cycles, versus at least 38 for a
            // word stack load, and never uses more bytes. Prefer upper donors.
            for (unsigned I : {4u, 5u, 6u, 7u, 0u, 1u, 2u, 3u}) {
              if (!Values[I].equals(V))
                continue;
              unsigned Move = Reg >= 4 && I >= 4 ? AVM::MOV : AVM::MOV_RR;
              BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Move))
                  .add(MI.getOperand(0))
                  .addReg(Words[I]);
              ChangedReg(Words[I]);
              Remove = true;
              break;
            }
          }
          Results.emplace_back(Reg, V);
        } else {
          WordValue V = MI.getOperand(1).isUndef() ? Fresh() : Values[Reg];
          Remove = Found != Stack.end() && Found->second.equals(V);
          for (auto S = Stack.begin(); S != Stack.end();) {
            auto Current = S++;
            if (Current->first < At + 2 && At < Current->first + 2)
              Stack.erase(Current);
          }
          Stack[At] = V;
        }
      }
    }
    // A pure operation may be removed only if it has no additional defs or
    // unmodeled effects. Ordered memory operations were excluded above.
    if (Remove && !HandledStack) {
      if (MI.mayLoadOrStore() || MI.hasUnmodeledSideEffects())
        Remove = false;
      for (const MachineOperand &MO : MI.operands())
        if (MO.isRegMask() ||
            (MO.isReg() && MO.isDef() && MO.getReg() &&
             !TRI.regsOverlap(MO.getReg(), MI.getOperand(0).getReg())))
          Remove = false;
    }
    if (!HandledStack &&
        (MI.mayStore() || MI.isCall() || MI.hasUnmodeledSideEffects() ||
         MI.hasOrderedMemoryRef()))
      Stack.clear();
    if (MI.modifiesRegister(AVM::SP, &TRI))
      Stack.clear();
    if (!Remove) {
      bool UnknownClobbers =
          MI.hasUnmodeledSideEffects() &&
          llvm::none_of(MI.operands(), [](const MachineOperand &MO) {
            return MO.isRegMask();
          });
      for (unsigned I = 0; I != 8; ++I)
        if (UnknownClobbers || MI.modifiesRegister(Words[I], &TRI))
          Values[I] = Fresh();
    }
    for (const auto &[I, V] : Results)
      Values[I] = V;
    if (Remove) {
      for (const MachineOperand &MO : MI.operands())
        if (MO.isReg() && MO.getReg())
          ChangedReg(MO.getReg());
      MI.eraseFromParent();
      Changed = true;
    }
  }
  // Reusing a physical value can extend a lifetime past an old kill or dead
  // marker. Allocation is finished; clear only flags for affected registers.
  if (ChangedRegisters)
    for (MachineInstr &MI : MBB)
      for (MachineOperand &MO : MI.operands()) {
        if (!MO.isReg() || !MO.getReg())
          continue;
        for (unsigned I = 0; I != 8; ++I) {
          if (!(ChangedRegisters & (1u << I)) ||
              !TRI.regsOverlap(MO.getReg(), Words[I]))
            continue;
          if (MO.isUse())
            MO.setIsKill(false);
          else if (MO.isDef())
            MO.setIsDead(false);
          break;
        }
      }
  return Changed;
}

// A proven redundant normalization can leave a copy used only by a compare.
// Forward that use after allocation, accounting for a slower compare when its
// donor is a lower register. No register assignment or spill decision changes.
static bool forwardCompareCopies(MachineBasicBlock &MBB,
                                  const AVMInstrInfo &TII) {
  const auto &TRI = TII.getRegisterInfo();
  LivePhysRegs Live(TRI);
  Live.addLiveOuts(MBB);
  bool Changed = false;
  for (auto It = MBB.end(); It != MBB.begin();) {
    MachineInstr &MI = *--It;
    unsigned Opcode = MI.getOpcode();
    bool Immediate = Opcode == AVM::CMPIS8 || Opcode == AVM::COLDCMPIS8;
    bool Binary = Opcode == AVM::CMP || Opcode == AVM::CMP_RR;
    bool Test = Opcode == AVM::TST8 || Opcode == AVM::TST16;
    if (!MI.isBundled() && (Immediate || Binary || Test)) {
      for (;;) {
        auto Copy = MI.getIterator();
        while (Copy != MBB.begin()) {
          --Copy;
          if (!Copy->isDebugInstr())
            break;
        }
        if (Copy == MI.getIterator() ||
            Copy->isBundled() ||
            (Copy->getOpcode() != AVM::MOV && Copy->getOpcode() != AVM::MOV_RR) ||
            Copy->getOperand(1).isUndef())
          break;
        Register D = Copy->getOperand(0).getReg();
        Register S = Copy->getOperand(1).getReg();
        if (D == S || Live.contains(D))
          break;
        unsigned Count = Binary ? 2 : 1;
        bool Reads = false;
        bool Upper = true;
        for (unsigned I = 0; I != Count; ++I) {
          Register R = MI.getOperand(I).getReg();
          Reads |= R == D;
          Upper &= AVM::UpperGPR16RegClass.contains(R == D ? S : R);
        }
        if (!Reads)
          break;
        // The maximum compare penalty is 21 cycles. A full-register copy
        // costs 37; an upper-register copy leaves the compare compact.
        for (unsigned I = 0; I != Count; ++I)
          if (MI.getOperand(I).getReg() == D) {
            MI.getOperand(I).setReg(S);
            MI.getOperand(I).setIsKill(false);
          }
        if (Immediate)
          MI.setDesc(TII.get(Upper ? AVM::CMPIS8 : AVM::COLDCMPIS8));
        else if (Binary)
          MI.setDesc(TII.get(Upper ? AVM::CMP : AVM::CMP_RR));
        MBB.erase(Copy);
        Changed = true;
      }
    }
    Live.stepBackward(MI);
  }
  if (Changed)
    for (MachineInstr &MI : MBB)
      for (MachineOperand &MO : MI.operands())
        if (MO.isReg() && MO.isUse())
          MO.setIsKill(false);
  return Changed;
}

// Adjacent entries are complementary conditions. Keep these tables separate
// from the instruction encoding: opcode numbering is not a condition code.
static constexpr unsigned CSetOpcodes[] = {
    AVM::CSET_EQ, AVM::CSET_NE, AVM::CSET_ULT,
    AVM::CSET_UGE, AVM::CSET_SLT, AVM::CSET_SGE};
static constexpr unsigned CMovOpcodes[] = {
    AVM::CMOV_EQ, AVM::CMOV_NE, AVM::CMOV_ULT,
    AVM::CMOV_UGE, AVM::CMOV_SLT, AVM::CMOV_SGE};
static constexpr unsigned BranchOpcodes[] = {
    AVM::RELAX_BR_EQ, AVM::RELAX_BR_NE, AVM::RELAX_BR_ULT,
    AVM::RELAX_BR_UGE, AVM::RELAX_BR_SLT, AVM::RELAX_BR_SGE};

static std::optional<unsigned> csetCondition(unsigned Opcode) {
  for (unsigned I = 0; I != 6; ++I)
    if (Opcode == CSetOpcodes[I])
      return I;
  return std::nullopt;
}

static unsigned booleanFlagsConsumer(unsigned Opcode, unsigned Condition) {
  for (const unsigned *Opcodes : {CMovOpcodes, BranchOpcodes, CSetOpcodes}) {
    if (Opcode == Opcodes[0])
      return Opcodes[Condition ^ 1]; // Boolean == 0.
    if (Opcode == Opcodes[1])
      return Opcodes[Condition]; // Boolean != 0.
  }
  return 0;
}

// CSET preserves CC, as do ordinary arithmetic and logical instructions. A
// zero test of its unmodified Boolean can therefore use the original flags,
// provided every reader of the test's flags can be translated as well.
static bool reuseBooleanCompareFlags(MachineBasicBlock &MBB,
                                     const AVMInstrInfo &TII) {
  if (!MBB.getParent()->getProperties().hasProperty(
          MachineFunctionProperties::Property::TracksLiveness))
    return false;
  const AVMRegisterInfo &TRI = TII.getRegisterInfo();
  LivePhysRegs LiveOut(TRI);
  LiveOut.addLiveOuts(MBB);
  bool Changed = false;
  for (auto It = MBB.begin(); It != MBB.end();) {
    auto Test = It++;
    if (Test->getOpcode() != AVM::TST8 && Test->getOpcode() != AVM::TST16)
      continue;
    Register Bool = Test->getOperand(0).getReg();
    auto Set = Test;
    std::optional<unsigned> Condition;
    while (Set != MBB.begin()) {
      --Set;
      if (Set->isMetaInstruction())
        continue;
      if (Set->isCall() || Set->isInlineAsm() ||
          Set->hasUnmodeledSideEffects() || Set->modifiesRegister(AVM::CC, &TRI))
        break;
      if (Set->modifiesRegister(Bool, &TRI)) {
        if (Set->getOperand(0).isReg() && Set->getOperand(0).getReg() == Bool)
          Condition = csetCondition(Set->getOpcode());
        break;
      }
    }
    if (!Condition)
      continue;

    SmallVector<std::pair<MachineInstr *, unsigned>, 4> Consumers;
    auto End = It;
    bool Safe = true;
    for (; End != MBB.end(); ++End) {
      if (End->isMetaInstruction())
        continue;
      if (End->isInlineAsm() || End->hasUnmodeledSideEffects()) {
        Safe = false;
        break;
      }
      if (End->readsRegister(AVM::CC, &TRI)) {
        unsigned Opcode = booleanFlagsConsumer(End->getOpcode(), *Condition);
        if (!Opcode) {
          Safe = false;
          break;
        }
        Consumers.emplace_back(&*End, Opcode);
      }
      if (End->modifiesRegister(AVM::CC, &TRI) || End->isCall())
        break;
    }
    if (!Safe || Consumers.empty() ||
        (End == MBB.end() && LiveOut.contains(AVM::CC)))
      continue;

    for (auto [Consumer, Opcode] : Consumers)
      Consumer->setDesc(TII.get(Opcode));
    // The original flags now live through CSET and any intervening readers.
    // Leave CSET itself for machine DCE, which keeps it if the Boolean also
    // escapes as a data value (including through a register-pair alias).
    for (auto Scan = Set; Scan != End; ++Scan)
      Scan->clearRegisterKills(AVM::CC, &TRI);
    MBB.erase(Test);
    Changed = true;
  }
  return Changed;
}

static unsigned reversedCompareCMov(unsigned Opcode) {
  switch (Opcode) {
  case AVM::CMOV_EQ:
  case AVM::CMOV_NE:
    return Opcode;
  case AVM::CMOV_ULT:
    return AVM::CMOV_UGE;
  case AVM::CMOV_UGE:
    return AVM::CMOV_ULT;
  case AVM::CMOV_SLT:
    return AVM::CMOV_SGE;
  case AVM::CMOV_SGE:
    return AVM::CMOV_SLT;
  default:
    return 0;
  }
}

static bool reuseReversedCompare(MachineBasicBlock &MBB,
                                 const AVMInstrInfo &TII) {
  const AVMRegisterInfo &TRI = TII.getRegisterInfo();
  MachineInstr *Previous = nullptr;
  Register LHS, RHS;
  bool Changed = false;

  for (auto It = MBB.begin(); It != MBB.end();) {
    auto Current = It++;
    MachineInstr &MI = *Current;
    if (MI.isDebugInstr() || MI.isMetaInstruction())
      continue;
    if (isWordCompare(MI)) {
      Register NewLHS = MI.getOperand(0).getReg();
      Register NewRHS = MI.getOperand(1).getReg();
      if (Previous && Previous->getOpcode() == MI.getOpcode() &&
          NewLHS == RHS && NewRHS == LHS) {
        auto Next = It;
        while (Next != MBB.end() && Next->isDebugInstr())
          ++Next;
        if (Next != MBB.end()) {
          unsigned Inverse = reversedCompareCMov(Next->getOpcode());
          if (Inverse &&
              (Inverse == Next->getOpcode() ||
               (Next->getOperand(0).getReg() == LHS &&
                Next->getOperand(2).getReg() == RHS))) {
            Next->setDesc(TII.get(Inverse));
            MBB.erase(Current);
            Changed = true;
            continue;
          }
        }
      }
      Previous = &MI;
      LHS = NewLHS;
      RHS = NewRHS;
      continue;
    }
    if (Previous &&
        (MI.modifiesRegister(AVM::CC, &TRI) ||
         MI.modifiesRegister(LHS, &TRI) ||
         MI.modifiesRegister(RHS, &TRI) || MI.isCall() ||
         MI.hasUnmodeledSideEffects()))
      Previous = nullptr;
  }
  return Changed;
}

// A mask built from bit 1 as (x << 14) >>s 15 selects either all of x or
// zero. Testing bit 1 and using a conditional move is cheaper than the two
// long immediate shifts, provided that the new test cannot change live CC.
static bool selectByBitOne(MachineBasicBlock &MBB, const AVMInstrInfo &TII) {
  const AVMRegisterInfo &TRI = TII.getRegisterInfo();
  bool Changed = false;
  for (auto It = MBB.begin(); It != MBB.end();) {
    auto Move = It++;
    if (Move->getOpcode() != AVM::MOV && Move->getOpcode() != AVM::MOV_RR)
      continue;
    Register Dest = Move->getOperand(0).getReg();
    Register Src = Move->getOperand(1).getReg();
    if (Dest == Src || !AVM::UpperGPR16RegClass.contains(Dest))
      continue;

    auto ShiftLeft = It;
    auto ShiftRight = ShiftLeft == MBB.end() ? MBB.end() : std::next(ShiftLeft);
    auto And = ShiftRight == MBB.end() ? MBB.end() : std::next(ShiftRight);
    if (And == MBB.end() || ShiftLeft->getOpcode() != AVM::LSL16I ||
        ShiftRight->getOpcode() != AVM::ASR16I ||
        (And->getOpcode() != AVM::AND && And->getOpcode() != AVM::AND_RR) ||
        ShiftLeft->getOperand(0).getReg() != Dest ||
        ShiftLeft->getOperand(1).getReg() != Dest ||
        ShiftLeft->getOperand(2).getImm() != 14 ||
        ShiftRight->getOperand(0).getReg() != Dest ||
        ShiftRight->getOperand(1).getReg() != Dest ||
        ShiftRight->getOperand(2).getImm() != 15 ||
        And->getOperand(0).getReg() != Dest ||
        And->getOperand(1).getReg() != Dest ||
        And->getOperand(2).getReg() != Src)
      continue;

    bool CCLive = false;
    bool CCOverwritten = false;
    for (auto Scan = std::next(And); Scan != MBB.end(); ++Scan) {
      if (Scan->readsRegister(AVM::CC, &TRI)) {
        CCLive = true;
        break;
      }
      if (Scan->modifiesRegister(AVM::CC, &TRI)) {
        CCOverwritten = true;
        break;
      }
    }
    if (CCLive || (!CCOverwritten && !MBB.succ_empty()))
      continue;

    DebugLoc DL = Move->getDebugLoc();
    BuildMI(MBB, Move, DL, TII.get(AVM::LDI8), Dest).addImm(2);
    BuildMI(MBB, Move, DL,
            TII.get(AVM::UpperGPR16RegClass.contains(Src) ? AVM::AND
                                                          : AVM::AND_RR),
            Dest)
        .addReg(Dest)
        .addReg(Src);
    BuildMI(MBB, Move, DL, TII.get(AVM::TST8)).addReg(Dest);
    BuildMI(MBB, Move, DL, TII.get(AVM::XOR), Dest)
        .addReg(Dest)
        .addReg(Dest);
    BuildMI(MBB, Move, DL, TII.get(AVM::CMOV_NE), Dest)
        .addReg(Dest)
        .addReg(Src);
    It = std::next(And);
    MBB.erase(And);
    MBB.erase(ShiftRight);
    MBB.erase(ShiftLeft);
    MBB.erase(Move);
    Changed = true;
  }
  return Changed;
}

// Recover a conditional expensive calculation without changing allocation.
// Both outcomes must pay less than the original CMOV sequence. In particular,
// duplicate a bounded return tail rather than adding a merge jump to either
// outcome. The cheap CMOV and its register assignment otherwise stay intact.
static bool branchAroundExpensiveSelect(MachineBasicBlock &MBB,
                                        const AVMInstrInfo &TII) {
  MachineFunction &MF = *MBB.getParent();
  if (MF.getFunction().hasOptSize() ||
      MF.getFunction().hasFnAttribute(Attribute::StrictFP))
    return false;
  const AVMRegisterInfo &TRI = TII.getRegisterInfo();
  using AVM::AVMCostKind;
  unsigned BranchTaken = AVM::getBranchCycles(AVMCostKind::BrEq16, true);
  unsigned BranchNotTaken = AVM::getBranchCycles(AVMCostKind::BrEq16, false);
  unsigned CopyCycles = AVM::getFixedCycles(AVMCostKind::Mov32Full);
  unsigned CMovTaken = 2 * AVM::getBranchCycles(AVMCostKind::CmovEq, true);
  unsigned CMovNotTaken = 2 * AVM::getBranchCycles(AVMCostKind::CmovEq, false);

  // Appending a return tail cannot lengthen any existing branch. Bound the
  // new branch to the long encoding, whose costs are used above.
  unsigned FunctionBytes = 0;
  for (const MachineBasicBlock &BB : MF)
    for (const MachineInstr &MI : BB)
      FunctionBytes += TII.getInstSizeInBytes(MI);
  if (FunctionBytes + 32 >= INT16_MAX || BranchNotTaken >= CMovNotTaken)
    return false;

  for (auto Op = MBB.begin(); Op != MBB.end(); ++Op) {
    AVM::AVMCycleRange Cycles;
    switch (Op->getOpcode()) {
    case AVM::FADD:
      Cycles = AVM::getCycleRange(AVMCostKind::FAdd);
      break;
    case AVM::FSUB:
      Cycles = AVM::getCycleRange(AVMCostKind::FSub);
      break;
    case AVM::FMUL:
      Cycles = AVM::getCycleRange(AVMCostKind::FMul);
      break;
    case AVM::FDIV:
      Cycles = AVM::getCycleRange(AVMCostKind::FDiv);
      break;
    default:
      continue; // Other operations need a validated minimum-cycle estimate.
    }
    // Use the instruction timing model, not an IR opcode or source-function
    // name. Currently FDIV is the only native pair operation above this bound.
    if (Op->isMetaInstruction() || Op->isDebugInstr() ||
        Op->getNumExplicitDefs() != 1 || Op->mayLoadOrStore() ||
        Op->isCall() || Op->isTerminator() || Op->isInlineAsm() ||
        Op->hasUnmodeledSideEffects() || Op->modifiesRegister(AVM::CC, &TRI) ||
        Cycles.Typical <= 2 * BranchTaken + CopyCycles)
      continue;
    Register Dest = Op->getOperand(0).getReg();
    if (!AVM::GPR32RegClass.contains(Dest))
      continue;
    if (BranchTaken + CopyCycles >= Cycles.Minimum + CMovTaken)
      continue;

    auto Test = std::next(Op);
    MachineInstr *ConditionLoad = nullptr;
    if (Test != MBB.end() && isWordStackLoad(*Test)) {
      ConditionLoad = &*Test;
      ++Test;
    }
    if (Test == MBB.end() ||
        (Test->getOpcode() != AVM::TST8 && Test->getOpcode() != AVM::TST16) ||
        Test->readsRegister(Dest, &TRI))
      continue;
    auto Low = std::next(Test);
    auto High = Low == MBB.end() ? MBB.end() : std::next(Low);
    if (High == MBB.end())
      continue;
    unsigned Condition = 0;
    while (Condition < 6 && Low->getOpcode() != CMovOpcodes[Condition])
      ++Condition;
    if (Condition == 6 || High->getOpcode() != Low->getOpcode() ||
        Low->getOperand(0).getReg() != TRI.getSubReg(Dest, AVM::sub_lo16) ||
        High->getOperand(0).getReg() != TRI.getSubReg(Dest, AVM::sub_hi16) ||
        Low->getOperand(1).getReg() != Low->getOperand(0).getReg() ||
        High->getOperand(1).getReg() != High->getOperand(0).getReg())
      continue;
    Register Source = TRI.getMatchingSuperReg(Low->getOperand(2).getReg(),
                                               AVM::sub_lo16,
                                               &AVM::GPR32RegClass);
    if (!Source || High->getOperand(2).getReg() !=
                       TRI.getSubReg(Source, AVM::sub_hi16) ||
        TRI.regsOverlap(Source, Dest))
      continue;

    MachineInstr *OperandCopy = nullptr;
    Register ConditionReg;
    if (ConditionLoad) {
      Register Loaded = ConditionLoad->getOperand(0).getReg();
      if (TRI.regsOverlap(Source, Loaded) ||
          Test->getOperand(0).getReg() != Loaded ||
          ConditionLoad->hasOrderedMemoryRef())
        continue;
      if (Op->readsRegister(Loaded, &TRI) ||
          Op->modifiesRegister(Loaded, &TRI)) {
        // A stacked condition may reuse the divisor register after FDIV.
        // Reuse the result's low word before its existing operand copy instead.
        // This needs no scratch register or spill, and the cost check includes
        // the more expensive full-register load encoding when necessary.
        if (Op == MBB.begin() || FunctionBytes + 32 >= 128)
          continue;
        auto Copy = std::prev(Op);
        if (Copy->getOpcode() != AVM::MOV32_F2 ||
            Copy->getOperand(0).getReg() != Dest ||
            Copy->getOperand(1).getReg() != Source)
          continue;
        unsigned LoadPenalty = AVM::getFixedCycles(AVMCostKind::LdSp16Cold) -
                               TII.getInstrLatency(nullptr, *ConditionLoad);
        if (AVM::getBranchCycles(AVMCostKind::BrEq8, false) + LoadPenalty >=
            CMovNotTaken)
          continue;
        OperandCopy = &*Copy;
        ConditionReg = TRI.getSubReg(Dest, AVM::sub_lo16);
      }
    }

    SmallVector<const MachineInstr *, 8> Tail;
    MachineBasicBlock *BB = &MBB;
    auto It = std::next(High);
    unsigned Bytes = 0;
    unsigned Instructions = 0;
    bool HasReturn = false;
    for (unsigned Blocks = 0; Blocks != 2 && !HasReturn; ++Blocks) {
      for (; It != BB->end(); ++It) {
        if (It->isDebugInstr())
          continue;
        if (It->isCFIInstruction()) {
          auto Kind = MF.getFrameInstructions()[It->getOperand(0).getCFIIndex()]
                          .getOperation();
          if (Kind == MCCFIInstruction::OpRememberState ||
              Kind == MCCFIInstruction::OpRestoreState)
            break;
          Tail.push_back(&*It);
          continue;
        }
        if (It->isMetaInstruction() || It->isCall() || It->isBranch() ||
            It->isInlineAsm() || It->isNotDuplicable())
          break;
        Bytes += TII.getInstSizeInBytes(*It);
        ++Instructions;
        Tail.push_back(&*It);
        HasReturn = It->isReturn();
        if (HasReturn || Bytes > 16 || Instructions > 8)
          break;
      }
      if (HasReturn || It != BB->end() || BB->succ_size() != 1 ||
          BB->getNextNode() != *BB->succ_begin())
        break;
      BB = *BB->succ_begin();
      It = BB->begin();
    }
    if (!HasReturn || Bytes > 16 || Instructions > 8)
      continue;

    DebugLoc DL = Op->getDebugLoc();
    auto *Divide = MF.CreateMachineBasicBlock(MBB.getBasicBlock());
    auto *Bypass = MF.CreateMachineBasicBlock(MBB.getBasicBlock());
    MF.insert(std::next(MBB.getIterator()), Divide);
    MF.push_back(Bypass);
    // The copied epilogue starts with the unwind state from before the
    // original epilogue, rather than the state after its final register pop.
    if (llvm::any_of(Tail, [](const MachineInstr *MI) {
          return MI->isCFIInstruction();
        })) {
      auto *First = const_cast<MachineInstr *>(Tail.front());
      unsigned Remember = MF.addFrameInst(
          MCCFIInstruction::createRememberState(nullptr));
      unsigned Restore = MF.addFrameInst(
          MCCFIInstruction::createRestoreState(nullptr));
      BuildMI(*First->getParent(), First, DL, TII.get(TargetOpcode::CFI_INSTRUCTION))
          .addCFIIndex(Remember);
      BuildMI(*Bypass, Bypass->end(), DL, TII.get(TargetOpcode::CFI_INSTRUCTION))
          .addCFIIndex(Restore);
    }
    BuildMI(*Bypass, Bypass->end(), DL, TII.get(AVM::MOV32_F2), Dest)
        .addReg(Source);
    for (const MachineInstr *MI : Tail)
      Bypass->insert(Bypass->end(), MF.CloneMachineInstr(MI));
    auto Split = OperandCopy ? OperandCopy->getIterator() : Op;
    if (ConditionLoad) {
      if (ConditionReg) {
        ConditionLoad->setDesc(TII.get(AVM::LDSP16));
        ConditionLoad->getOperand(0).setReg(ConditionReg);
        Test->getOperand(0).setReg(ConditionReg);
      }
      MBB.splice(Split, &MBB, ConditionLoad->getIterator());
    }
    MBB.splice(Split, &MBB, Test);
    Divide->splice(Divide->end(), &MBB, Split, MBB.end());
    Low->eraseFromParent();
    High->eraseFromParent();
    Divide->transferSuccessorsAndUpdatePHIs(&MBB);
    MBB.addSuccessor(Divide);
    MBB.addSuccessor(Bypass);
    BuildMI(MBB, MBB.end(), DL, TII.get(BranchOpcodes[Condition]))
        .addMBB(Bypass);
    return true;
  }
  return false;
}

class AVMLatePeephole final : public MachineFunctionPass {
public:
  static char ID;
  AVMLatePeephole() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return PASS_NAME; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    const AVMInstrInfo &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
    bool Changed = false;
    for (MachineBasicBlock &MBB : MF) {
      Changed |= removeRedundantStackReloads(MBB, TII.getRegisterInfo());
      Changed |= reuseReversedCompare(MBB, TII);
      if (!MF.getFunction().hasOptSize())
        Changed |= selectByBitOne(MBB, TII);
      Changed |= reuseBooleanCompareFlags(MBB, TII);
    }
    AVMRegisterRange Ranges(MF, TII);
    for (MachineBasicBlock &MBB : MF) {
      Changed |= cleanRegisterAndStackValues(MBB, TII, Ranges);
      Changed |= forwardCompareCopies(MBB, TII);
    }
    SmallVector<MachineBasicBlock *, 16> Blocks;
    for (MachineBasicBlock &MBB : MF)
      Blocks.push_back(&MBB);
    bool ChangedCFG = false;
    for (MachineBasicBlock *MBB : Blocks)
      if (branchAroundExpensiveSelect(*MBB, TII)) {
        ChangedCFG = true;
        break; // Bound code growth and the number of copied unwind states.
      }
    if (ChangedCFG) {
      Blocks.clear();
      for (MachineBasicBlock &MBB : MF) {
        Blocks.push_back(&MBB);
        for (MachineInstr &MI : MBB)
          for (MachineOperand &MO : MI.operands())
            if (MO.isReg() && MO.isUse())
              MO.setIsKill(false);
      }
      fullyRecomputeLiveIns(Blocks);
    }
    Changed |= ChangedCFG;
    return Changed;
  }
};

} // namespace

char AVMLatePeephole::ID = 0;

INITIALIZE_PASS(AVMLatePeephole, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMLatePeepholePass() {
  return new AVMLatePeephole();
}
