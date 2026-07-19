//===-- AVMExpandPseudoInsts.cpp - Expand AVM semantic pseudos ------------===//

#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/MathExtras.h"

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
      // Both choices replace the same instruction in the same block, so its
      // block frequency is a common factor in their measured cycle costs.
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
        case AVM::COPY32_PSEUDO: {
          Register Dest = MI.getOperand(0).getReg();
          Register Src = MI.getOperand(1).getReg();
          if (AVM::UpperGPR32RegClass.contains(Dest, Src)) {
            const AVMRegisterInfo &TRI = TII.getRegisterInfo();
            Register DestLo = TRI.getSubReg(Dest, AVM::sub_lo16);
            Register DestHi = TRI.getSubReg(Dest, AVM::sub_hi16);
            Register SrcLo = TRI.getSubReg(Src, AVM::sub_lo16);
            Register SrcHi = TRI.getSubReg(Src, AVM::sub_hi16);
            unsigned Kill = MI.getOperand(1).isKill() ? RegState::Kill : 0;
            BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::MOV), DestLo)
                .addReg(SrcLo, Kill);
            BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::MOV), DestHi)
                .addReg(SrcHi, Kill);
            MI.eraseFromParent();
            Changed = true;
            continue;
          }
          NewOpcode = AVM::MOV32_F2;
          break;
        }
        case AVM::SEXT8_PSEUDO:
          NewOpcode = AVM::SEXT8;
          break;
        case AVM::ZEXT8_PSEUDO:
          NewOpcode = AVM::ZEXT8;
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
        case AVM::CALL_DIRECT_PSEUDO:
          NewOpcode = AVM::RELAX_CALL;
          for (unsigned I = 1; I != MI.getNumOperands(); ++I)
            if (MachineOperand &MO = MI.getOperand(I);
                MO.isReg() && !MO.isImplicit())
              MO.setImplicit();
          break;
        case AVM::CALL_INDIRECT_PSEUDO:
          NewOpcode = AVM::CALLP;
          for (unsigned I = 1; I != MI.getNumOperands(); ++I)
            if (MachineOperand &MO = MI.getOperand(I);
                MO.isReg() && !MO.isImplicit())
              MO.setImplicit();
          break;
        case AVM::DATA_ADDR_PSEUDO:
          NewOpcode =
              IsUpper(MI.getOperand(0).getReg()) ? AVM::LDI16 : AVM::COLDLDI16;
          break;
        case AVM::LOAD8U_PSEUDO: {
          Register Dest = MI.getOperand(0).getReg();
          Register Addr = MI.getOperand(1).getReg();
          NewOpcode = IsUpper(Addr) ? (IsUpper(Dest) ? AVM::LD8U : AVM::F5LD8U)
                                    : AVM::GPLD8U;
          break;
        }
        case AVM::LOAD16_PSEUDO: {
          Register Dest = MI.getOperand(0).getReg();
          Register Addr = MI.getOperand(1).getReg();
          NewOpcode = IsUpper(Addr) ? (IsUpper(Dest) ? AVM::LD16 : AVM::F5LD16)
                                    : AVM::GPLD16;
          break;
        }
        case AVM::STORE8_PSEUDO: {
          Register Addr = MI.getOperand(0).getReg();
          Register Src = MI.getOperand(1).getReg();
          NewOpcode = IsUpper(Addr) ? (IsUpper(Src) ? AVM::ST8 : AVM::F3ST8)
                                    : AVM::GPST8;
          break;
        }
        case AVM::STORE16_PSEUDO: {
          Register Addr = MI.getOperand(0).getReg();
          Register Src = MI.getOperand(1).getReg();
          NewOpcode = IsUpper(Addr) ? (IsUpper(Src) ? AVM::ST16 : AVM::F5ST16)
                                    : AVM::GPST16;
          break;
        }
        case AVM::LOAD32_PSEUDO:
          NewOpcode = AVM::LD32;
          break;
        case AVM::STORE32_PSEUDO:
          NewOpcode = AVM::ST32;
          break;
        case AVM::LOAD8U_POST_PSEUDO:
          NewOpcode = IsUpper(MI.getOperand(2).getReg()) ? AVM::F7LD8U_POST
                                                         : AVM::GPLD8U_POST;
          break;
        case AVM::LOAD16_POST_PSEUDO:
          NewOpcode = IsUpper(MI.getOperand(2).getReg()) ? AVM::F7LD16_POST
                                                         : AVM::GPLD16_POST;
          break;
        case AVM::STORE8_POST_PSEUDO:
          NewOpcode = IsUpper(MI.getOperand(1).getReg()) ? AVM::F6ST8_POST
                                                         : AVM::GPST8_POST;
          break;
        case AVM::STORE16_POST_PSEUDO:
          NewOpcode = IsUpper(MI.getOperand(1).getReg()) ? AVM::F7ST16_POST
                                                         : AVM::GPST16_POST;
          break;
        case AVM::ABS_LOAD8U_PSEUDO:
          NewOpcode = AVM::LDM8U;
          break;
        case AVM::ABS_LOAD16_PSEUDO:
          NewOpcode = AVM::LDM16;
          break;
        case AVM::ABS_STORE8_PSEUDO:
          NewOpcode = AVM::STM8;
          break;
        case AVM::ABS_STORE16_PSEUDO:
          NewOpcode = AVM::STM16;
          break;
        case AVM::OUT_STORE8_PSEUDO: {
          bool Compact = IsUpper(MI.getOperand(1).getReg()) &&
                         isUInt<4>(MI.getOperand(0).getImm());
          NewOpcode = PreferCompact(Compact, AVM::STSP8_COMPACT,
                                    AVM::AVMCostKind::StSp8Short, AVM::STSP8,
                                    AVM::AVMCostKind::StSp8Cold);
          break;
        }
        case AVM::OUT_STORE16_PSEUDO: {
          bool Compact = IsUpper(MI.getOperand(1).getReg()) &&
                         isUInt<4>(MI.getOperand(0).getImm());
          NewOpcode = PreferCompact(Compact, AVM::STSP16_COMPACT,
                                    AVM::AVMCostKind::StSp16Short, AVM::STSP16,
                                    AVM::AVMCostKind::StSp16Cold);
          break;
        }
        case AVM::OUT_STORE24_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Pair = MI.getOperand(1).getReg();
          Register Lo = TRI.getSubReg(Pair, AVM::sub_lo16);
          Register Hi = TRI.getSubReg(Pair, AVM::sub_hi16);
          uint64_t Offset = MI.getOperand(0).getImm();
          if (!isUInt<8>(Offset + 2))
            report_fatal_error("AVM outgoing pointer offset exceeds u8");
          unsigned Kill = MI.getOperand(1).isKill() ? RegState::Kill : 0;
          unsigned LoOpcode =
              PreferCompact(IsUpper(Lo) && isUInt<4>(Offset),
                            AVM::STSP16_COMPACT, AVM::AVMCostKind::StSp16Short,
                            AVM::STSP16, AVM::AVMCostKind::StSp16Cold);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(LoOpcode))
              .addImm(Offset)
              .addReg(Lo, Kill)
              .cloneMemRefs(MI);
          unsigned HiOpcode =
              PreferCompact(IsUpper(Hi) && isUInt<4>(Offset + 2),
                            AVM::STSP8_COMPACT, AVM::AVMCostKind::StSp8Short,
                            AVM::STSP8, AVM::AVMCostKind::StSp8Cold);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(HiOpcode))
              .addImm(Offset + 2)
              .addReg(Hi, Kill)
              .cloneMemRefs(MI);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::OUT_STORE32_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Pair = MI.getOperand(1).getReg();
          Register Lo = TRI.getSubReg(Pair, AVM::sub_lo16);
          Register Hi = TRI.getSubReg(Pair, AVM::sub_hi16);
          uint64_t Offset = MI.getOperand(0).getImm();
          if (!isUInt<8>(Offset + 2))
            report_fatal_error("AVM outgoing i32 argument offset exceeds u8");
          unsigned Kill = MI.getOperand(1).isKill() ? RegState::Kill : 0;
          auto EmitWord = [&](Register Reg, uint64_t WordOffset) {
            unsigned Opcode = PreferCompact(
                IsUpper(Reg) && isUInt<4>(WordOffset), AVM::STSP16_COMPACT,
                AVM::AVMCostKind::StSp16Short, AVM::STSP16,
                AVM::AVMCostKind::StSp16Cold);
            BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode))
                .addImm(WordOffset)
                .addReg(Reg, Kill)
                .cloneMemRefs(MI);
          };
          EmitWord(Lo, Offset);
          EmitWord(Hi, Offset + 2);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
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
