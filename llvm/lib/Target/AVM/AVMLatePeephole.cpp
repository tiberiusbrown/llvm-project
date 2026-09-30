//===-- AVMLatePeephole.cpp - Physical-register AVM cleanups ------------===//

#include "AVM.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/InitializePasses.h"
#include <iterator>

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

static bool removeDeadSpillRoundTrips(MachineBasicBlock &MBB,
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
    if (!Overwritten)
      continue;
    It = std::next(Load);
    MBB.erase(Load);
    MBB.erase(Store);
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

static bool removeRedundantByteExtends(MachineBasicBlock &MBB) {
  bool Changed = false;
  for (auto It = MBB.begin(); It != MBB.end();) {
    auto Producer = It++;
    if (!producesZeroExtendedByte(*Producer))
      continue;
    auto Extend = It;
    while (Extend != MBB.end() && Extend->isDebugInstr())
      ++Extend;
    if (Extend == MBB.end() || Extend->getOpcode() != AVM::ZEXT8 ||
        Producer->getOperand(0).getReg() != Extend->getOperand(0).getReg() ||
        Extend->getOperand(0).getReg() != Extend->getOperand(1).getReg())
      continue;
    It = std::next(Extend);
    MBB.erase(Extend);
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

class AVMLatePeephole final : public MachineFunctionPass {
public:
  static char ID;
  AVMLatePeephole() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return PASS_NAME; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    const AVMInstrInfo &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
    bool Changed = false;
    for (MachineBasicBlock &MBB : MF) {
      Changed |= removeRedundantByteExtends(MBB);
      Changed |= removeDeadSpillRoundTrips(MBB, TII.getRegisterInfo());
      Changed |= reuseReversedCompare(MBB, TII);
      if (!MF.getFunction().hasOptSize())
        Changed |= selectByBitOne(MBB, TII);
    }
    return Changed;
  }
};

} // namespace

char AVMLatePeephole::ID = 0;

INITIALIZE_PASS(AVMLatePeephole, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMLatePeepholePass() {
  return new AVMLatePeephole();
}
