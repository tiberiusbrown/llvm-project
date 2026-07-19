//===-- AVMExpandPseudoInsts.cpp - Expand AVM semantic pseudos ------------===//

#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/IR/Function.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "avm-expand-pseudo"
#define PASS_NAME "AVM post-RA pseudo instruction expansion"

namespace {
class AVMExpandPseudo final : public MachineFunctionPass {
public:
  static char ID;
  AVMExpandPseudo() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return PASS_NAME; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    const AVMInstrInfo &TII = *MF.getSubtarget<AVMSubtarget>().getInstrInfo();
    bool Changed = false;

    auto IsUpper = [](Register Reg) {
      return AVM::UpperGPR16RegClass.contains(Reg);
    };
    auto PreferCompact = [&](bool CompactLegal, unsigned CompactOpcode,
                             AVM::AVMCostKind CompactCost, unsigned FullOpcode,
                             AVM::AVMCostKind FullCost) {
      if (!CompactLegal)
        return FullOpcode;
      if (MF.getFunction().hasOptSize())
        return TII.get(CompactOpcode).getSize() <= TII.get(FullOpcode).getSize()
                   ? CompactOpcode
                   : FullOpcode;
      return AVM::getFixedCycles(CompactCost) <= AVM::getFixedCycles(FullCost)
                 ? CompactOpcode
                 : FullOpcode;
    };

    for (MachineBasicBlock &MBB : MF) {
      for (MachineInstr &MI : make_early_inc_range(MBB)) {
        unsigned NewOpcode = 0;
        switch (MI.getOpcode()) {
        case AVM::COPY16_PSEUDO:
          NewOpcode = PreferCompact(IsUpper(MI.getOperand(0).getReg()) &&
                                        IsUpper(MI.getOperand(1).getReg()),
                                    AVM::MOV, AVM::AVMCostKind::MovUpper,
                                    AVM::MOV_RR, AVM::AVMCostKind::MovFull);
          break;
        case AVM::LDI8_PSEUDO:
          NewOpcode =
              IsUpper(MI.getOperand(0).getReg()) ? AVM::LDI8 : AVM::COLDLDI8;
          break;
        case AVM::LDI16_PSEUDO:
          NewOpcode =
              IsUpper(MI.getOperand(0).getReg()) ? AVM::LDI16 : AVM::COLDLDI16;
          break;
        case AVM::ADD16_PSEUDO:
          NewOpcode = PreferCompact(IsUpper(MI.getOperand(0).getReg()) &&
                                        IsUpper(MI.getOperand(1).getReg()) &&
                                        IsUpper(MI.getOperand(2).getReg()),
                                    AVM::ADD, AVM::AVMCostKind::AddUpper,
                                    AVM::ADD_RR, AVM::AVMCostKind::AddFull);
          break;
        case AVM::SUB16_PSEUDO:
          NewOpcode = PreferCompact(IsUpper(MI.getOperand(0).getReg()) &&
                                        IsUpper(MI.getOperand(1).getReg()) &&
                                        IsUpper(MI.getOperand(2).getReg()),
                                    AVM::SUB, AVM::AVMCostKind::SubUpper,
                                    AVM::SUB_RR, AVM::AVMCostKind::SubFull);
          break;
        case AVM::AND16_PSEUDO:
          NewOpcode = PreferCompact(IsUpper(MI.getOperand(0).getReg()) &&
                                        IsUpper(MI.getOperand(1).getReg()) &&
                                        IsUpper(MI.getOperand(2).getReg()),
                                    AVM::AND, AVM::AVMCostKind::AndUpper,
                                    AVM::AND_RR, AVM::AVMCostKind::AndFull);
          break;
        case AVM::OR16_PSEUDO:
          NewOpcode = PreferCompact(IsUpper(MI.getOperand(0).getReg()) &&
                                        IsUpper(MI.getOperand(1).getReg()) &&
                                        IsUpper(MI.getOperand(2).getReg()),
                                    AVM::OR, AVM::AVMCostKind::OrUpper,
                                    AVM::OR_RR, AVM::AVMCostKind::OrFull);
          break;
        case AVM::XOR16_PSEUDO:
          NewOpcode = PreferCompact(IsUpper(MI.getOperand(0).getReg()) &&
                                        IsUpper(MI.getOperand(1).getReg()) &&
                                        IsUpper(MI.getOperand(2).getReg()),
                                    AVM::XOR, AVM::AVMCostKind::XorUpper,
                                    AVM::XOR_RR, AVM::AVMCostKind::XorFull);
          break;
        case AVM::RET_PSEUDO:
          NewOpcode = AVM::RET;
          break;
        default:
          continue;
        }

        MI.setDesc(TII.get(NewOpcode));
        Changed = true;
      }
    }
    return Changed;
  }
};
} // namespace

char AVMExpandPseudo::ID = 0;

INITIALIZE_PASS(AVMExpandPseudo, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMExpandPseudoPass() {
  return new AVMExpandPseudo();
}
