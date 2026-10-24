//===-- AVMAddressReuse.cpp - Reuse available data addresses --------------===//

#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/InitializePasses.h"
#include <array>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "avm-address-reuse"
#define PASS_NAME "AVM available data-address reuse"

namespace {
constexpr MCPhysReg Registers[] = {AVM::R0, AVM::R1, AVM::R2, AVM::R3,
                                   AVM::R4, AVM::R5, AVM::R6, AVM::R7};

// A 16-bit value is an opaque register value plus at most one data symbol
// and a constant. Roots identify values, not mutable physical registers.
struct Address {
  unsigned Root = 0;
  const GlobalValue *Symbol = nullptr;
  unsigned TargetFlags = 0;
  uint16_t Offset = 0;

  bool operator==(const Address &Other) const {
    return Root == Other.Root && Symbol == Other.Symbol &&
           TargetFlags == Other.TargetFlags && Offset == Other.Offset;
  }
};
using Addresses = std::array<Address, 8>;

static unsigned registerIndex(Register Reg) {
  for (unsigned I = 0; I != 8; ++I)
    if (Reg == Registers[I])
      return I;
  return 8;
}

static unsigned instructionCycles(unsigned Opcode) {
  using AVM::AVMCostKind;
  switch (Opcode) {
#define COST(Opcode, Kind)                                                     \
  case AVM::Opcode:                                                            \
    return AVM::getFixedCycles(AVMCostKind::Kind)
    COST(MOV, MovUpper);
    COST(MOV_RR, MovFull);
    COST(LDI8, Ldi8Upper);
    COST(LDI16, Ldi16Upper);
    COST(COLDLDI8, Ldi8Lower);
    COST(COLDLDI16, Ldi16Lower);
    COST(ADD, AddUpper);
    COST(ADD_RR, AddFull);
    COST(SUB, SubUpper);
    COST(SUB_RR, SubFull);
    COST(ADDIS8, AddIS8Upper);
    COST(COLDADDIS8, AddIS8Lower);
    COST(INC16, Inc16);
    COST(DEC16, Dec16);
    COST(LD8U, Ld8UUpper);
    COST(F5LD8U, Ld8UDense);
    COST(LD16, Ld16Upper);
    COST(F5LD16, Ld16Dense);
    COST(ST8, St8Upper);
    COST(F3ST8, St8Dense);
    COST(ST16, St16Upper);
    COST(F5ST16, St16Dense);
    COST(DPLD8U, Ld8UDisplaced);
    COST(DPLD16, Ld16Displaced);
    COST(DPST8, St8Displaced);
    COST(DPST16, St16Displaced);
#undef COST
  default:
    return 0;
  }
}

struct InstructionInfo {
  unsigned Uses = 0, Defs = 0, LiveAfter = 0;
  unsigned RemovableCycles = 0, Bytes = 0;
};

static InstructionInfo describe(const MachineInstr &MI,
                                const AVMRegisterInfo &TRI) {
  InstructionInfo Info;
  for (unsigned I = 0; I != 8; ++I) {
    if (MI.readsRegister(Registers[I], &TRI))
      Info.Uses |= 1u << I;
    if (MI.modifiesRegister(Registers[I], &TRI))
      Info.Defs |= 1u << I;
  }
  Info.Bytes = MI.getDesc().getSize();
  if (Info.Defs && !MI.mayLoadOrStore() && !MI.isCall() &&
      !MI.hasUnmodeledSideEffects())
    Info.RemovableCycles = instructionCycles(MI.getOpcode());
  return Info;
}

struct MemoryAccess {
  bool IsLoad, IsWord, Displaced;
  unsigned AddressOperand, ValueOperand;
};

static std::optional<MemoryAccess> memoryAccess(unsigned Opcode) {
  switch (Opcode) {
  case AVM::LD8U:
  case AVM::F5LD8U:
    return {{true, false, false, 1, 0}};
  case AVM::LD16:
  case AVM::F5LD16:
    return {{true, true, false, 1, 0}};
  case AVM::DPLD8U:
    return {{true, false, true, 1, 0}};
  case AVM::DPLD16:
    return {{true, true, true, 1, 0}};
  case AVM::ST8:
  case AVM::F3ST8:
    return {{false, false, false, 0, 1}};
  case AVM::ST16:
  case AVM::F5ST16:
    return {{false, true, false, 0, 1}};
  case AVM::DPST8:
    return {{false, false, true, 0, 2}};
  case AVM::DPST16:
    return {{false, true, true, 0, 2}};
  default:
    return std::nullopt;
  }
}

static unsigned memoryOpcode(const MemoryAccess &Access, Register Base,
                             Register Value, int Displacement) {
  if (Displacement || !AVM::UpperGPR16RegClass.contains(Base))
    return Access.IsLoad ? (Access.IsWord ? AVM::DPLD16 : AVM::DPLD8U)
                         : (Access.IsWord ? AVM::DPST16 : AVM::DPST8);
  bool Upper = AVM::UpperGPR16RegClass.contains(Value);
  if (Access.IsLoad)
    return Access.IsWord ? (Upper ? AVM::LD16 : AVM::F5LD16)
                         : (Upper ? AVM::LD8U : AVM::F5LD8U);
  return Access.IsWord ? (Upper ? AVM::ST16 : AVM::F5ST16)
                       : (Upper ? AVM::ST8 : AVM::F3ST8);
}

// Estimate the extra address instructions that the following machine DCE can
// delete. Compare both use sets so unrelated dead instructions earn no credit.
static std::pair<int, int>
savings(MachineInstr &MI, unsigned NewUses, unsigned NewOpcode,
        const DenseMap<const MachineInstr *, InstructionInfo> &Info,
        const AVMInstrInfo &TII) {
  InstructionInfo Current = Info.lookup(&MI);
  unsigned OldLive = (Current.LiveAfter & ~Current.Defs) | Current.Uses;
  unsigned NewLive = (Current.LiveAfter & ~Current.Defs) | NewUses;
  int Cycles = int(instructionCycles(MI.getOpcode())) -
               int(instructionCycles(NewOpcode));
  int Bytes = int(Current.Bytes) - int(TII.get(NewOpcode).getSize());
  auto It = MI.getIterator();
  for (unsigned Count = 0; It != MI.getParent()->begin() && Count < 32;) {
    const MachineInstr &Prev = *--It;
    if (Prev.isMetaInstruction())
      continue;
    if (Prev.isCall() || Prev.isInlineAsm() || Prev.hasUnmodeledSideEffects())
      break;
    ++Count;
    InstructionInfo P = Info.lookup(&Prev);
    bool OldDead = P.RemovableCycles && !(P.Defs & OldLive);
    bool NewDead = P.RemovableCycles && !(P.Defs & NewLive);
    Cycles += (int(NewDead) - int(OldDead)) * int(P.RemovableCycles);
    Bytes += (int(NewDead) - int(OldDead)) * int(P.Bytes);
    if (!OldDead)
      OldLive = (OldLive & ~P.Defs) | P.Uses;
    if (!NewDead)
      NewLive = (NewLive & ~P.Defs) | P.Uses;
  }
  return {Cycles, Bytes};
}

static std::optional<Address> definedAddress(const MachineInstr &MI,
                                             const Addresses &Values) {
  auto Operand = [&](unsigned I) -> std::optional<Address> {
    const MachineOperand &MO = MI.getOperand(I);
    unsigned Index = MO.isReg() ? registerIndex(MO.getReg()) : 8;
    if (Index == 8 || MO.isUndef())
      return std::nullopt;
    return Values[Index];
  };
  unsigned Opcode = MI.getOpcode();
  if (Opcode == AVM::MOV || Opcode == AVM::MOV_RR || MI.isCopy())
    return Operand(1);
  if (Opcode == AVM::LDI8 || Opcode == AVM::COLDLDI8 || Opcode == AVM::LDI16 ||
      Opcode == AVM::COLDLDI16) {
    const MachineOperand &MO = MI.getOperand(1);
    if (MO.isImm()) {
      uint16_t Immediate = MO.getImm();
      if (Opcode == AVM::LDI8 || Opcode == AVM::COLDLDI8)
        Immediate = uint8_t(Immediate);
      return Address{0, nullptr, 0, Immediate};
    }
    if (MO.isGlobal() && (Opcode == AVM::LDI16 || Opcode == AVM::COLDLDI16))
      return Address{0, MO.getGlobal(), MO.getTargetFlags(),
                     uint16_t(MO.getOffset())};
    return std::nullopt;
  }
  auto Left = MI.getNumOperands() > 1 ? Operand(1) : std::nullopt;
  if (!Left)
    return std::nullopt;
  if (Opcode == AVM::ADDIS8 || Opcode == AVM::COLDADDIS8 ||
      Opcode == AVM::INC16 || Opcode == AVM::DEC16) {
    int64_t Offset = Opcode == AVM::INC16   ? 1
                     : Opcode == AVM::DEC16 ? -1
                                            : MI.getOperand(2).getImm();
    Left->Offset += Offset;
    return Left;
  }
  if (Opcode != AVM::ADD && Opcode != AVM::ADD_RR && Opcode != AVM::SUB &&
      Opcode != AVM::SUB_RR)
    return std::nullopt;
  auto Right = Operand(2);
  if (!Right)
    return std::nullopt;
  if (Opcode == AVM::SUB || Opcode == AVM::SUB_RR) {
    if (Right->Root || Right->Symbol)
      return std::nullopt;
    Left->Offset -= Right->Offset;
    return Left;
  }
  if ((Left->Root && Right->Root) || (Left->Symbol && Right->Symbol))
    return std::nullopt;
  return Address{Left->Root ? Left->Root : Right->Root,
                 Left->Symbol ? Left->Symbol : Right->Symbol,
                 Left->Symbol ? Left->TargetFlags : Right->TargetFlags,
                 uint16_t(Left->Offset + Right->Offset)};
}

class AVMAddressReuse final : public MachineFunctionPass {
public:
  static char ID;
  AVMAddressReuse() : MachineFunctionPass(ID) {}
  StringRef getPassName() const override { return PASS_NAME; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    if (!MF.getProperties().hasProperty(
            MachineFunctionProperties::Property::TracksLiveness))
      return false;
    const auto &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
    const auto &TRI = TII.getRegisterInfo();
    DenseMap<const MachineInstr *, InstructionInfo> Info;
    for (MachineBasicBlock &MBB : MF) {
      LivePhysRegs Live(TRI);
      Live.addLiveOuts(MBB);
      for (MachineInstr &MI : llvm::reverse(MBB)) {
        InstructionInfo I = describe(MI, TRI);
        for (unsigned R = 0; R != 8; ++R)
          if (Live.contains(Registers[R]))
            I.LiveAfter |= 1u << R;
        Info[&MI] = I;
        Live.stepBackward(MI);
      }
    }

    unsigned NextRoot = 1;
    auto Fresh = [&]() { return Address{NextRoot++}; };
    DenseMap<const MachineBasicBlock *, Addresses> Outputs;
    bool Changed = false;
    // Propagate only fully known predecessor states. In particular, a loop
    // header with an unvisited backedge starts with fresh values rather than
    // assuming that registers retain their preheader values on every iteration.
    for (MachineBasicBlock *MBB :
         ReversePostOrderTraversal<MachineFunction *>(&MF)) {
      Addresses Values;
      for (Address &Value : Values)
        Value = Fresh();
      if (!MBB->pred_empty()) {
        auto First = Outputs.find(*MBB->pred_begin());
        bool AllSeen = llvm::all_of(MBB->predecessors(), [&](const auto *Pred) {
          return Outputs.count(Pred);
        });
        if (AllSeen) {
          for (unsigned R = 0; R != 8; ++R) {
            Address Value = First->second[R];
            if (llvm::all_of(MBB->predecessors(), [&](const auto *Pred) {
                  return Outputs.lookup(Pred)[R] == Value;
                }))
              Values[R] = Value;
          }
        }
      }

      for (auto It = MBB->begin(); It != MBB->end();) {
        MachineInstr *MI = &*It++;
        if (MI->isMetaInstruction())
          continue;
        InstructionInfo I = Info.lookup(MI);
        if (auto Access = memoryAccess(MI->getOpcode())) {
          unsigned OldBase =
              registerIndex(MI->getOperand(Access->AddressOperand).getReg());
          Address Wanted = Values[OldBase];
          if (Access->Displaced)
            Wanted.Offset +=
                MI->getOperand(Access->AddressOperand + 1).getImm();
          int BestCycles = 0, BestBytes = 0, BestDisplacement = 0;
          unsigned Best = 8, BestOpcode = 0;
          for (unsigned R = 0; R != 8; ++R) {
            // Reuse an already-live, unchanged register. No pointer hoisting
            // or longer live interval is needed after register allocation.
            if (R == OldBase || !(I.LiveAfter & (1u << R)) ||
                (I.Defs & (1u << R)))
              continue;
            const Address &Base = Values[R];
            if (Base.Root != Wanted.Root || Base.Symbol != Wanted.Symbol ||
                Base.TargetFlags != Wanted.TargetFlags)
              continue;
            int Displacement = int16_t(uint16_t(Wanted.Offset - Base.Offset));
            if (Displacement < -32 || Displacement > 223)
              continue;
            unsigned Opcode = memoryOpcode(
                *Access, Registers[R],
                MI->getOperand(Access->ValueOperand).getReg(), Displacement);
            unsigned Uses = 1u << R;
            if (!Access->IsLoad)
              Uses |= 1u << registerIndex(
                          MI->getOperand(Access->ValueOperand).getReg());
            auto [Cycles, Bytes] = savings(*MI, Uses, Opcode, Info, TII);
            bool OptSize = MF.getFunction().hasOptSize();
            if (Cycles < 0 || Bytes < 0 || (OptSize ? Bytes <= 0 : Cycles <= 0))
              continue;
            if (Best == 8 ||
                (OptSize ? Bytes > BestBytes : Cycles > BestCycles)) {
              Best = R;
              BestOpcode = Opcode;
              BestCycles = Cycles;
              BestBytes = Bytes;
              BestDisplacement = Displacement;
            }
          }
          if (Best != 8) {
            auto New =
                BuildMI(*MBB, MI, MI->getDebugLoc(), TII.get(BestOpcode));
            if (Access->IsLoad)
              New.add(MI->getOperand(Access->ValueOperand));
            New.addReg(Registers[Best]);
            if (memoryAccess(BestOpcode)->Displaced)
              New.addImm(BestDisplacement);
            if (!Access->IsLoad)
              New.add(MI->getOperand(Access->ValueOperand));
            New.cloneMemRefs(*MI).setMIFlags(MI->getFlags());
            New.copyImplicitOps(*MI);
            Info[New] = describe(*New, TRI);
            Info[New].LiveAfter = I.LiveAfter;
            MI->eraseFromParent();
            MI = New;
            Changed = true;
          }
        }

        auto Value = definedAddress(*MI, Values);
        if (MI->isCall() || MI->isInlineAsm() ||
            MI->hasUnmodeledSideEffects()) {
          for (Address &V : Values)
            V = Fresh();
          continue;
        }
        for (unsigned R = 0; R != 8; ++R)
          if (I.Defs & (1u << R))
            Values[R] = Fresh();
        if (Value && MI->getNumOperands() && MI->getOperand(0).isReg()) {
          unsigned R = registerIndex(MI->getOperand(0).getReg());
          if (R < 8)
            Values[R] = *Value;
        }
      }
      Outputs[MBB] = Values;
    }
    return Changed;
  }
};
} // namespace

char AVMAddressReuse::ID = 0;
INITIALIZE_PASS(AVMAddressReuse, DEBUG_TYPE, PASS_NAME, false, false)
FunctionPass *llvm::createAVMAddressReusePass() {
  return new AVMAddressReuse();
}
