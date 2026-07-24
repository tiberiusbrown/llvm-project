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
    auto EmitMove = [&](MachineBasicBlock &MBB, MachineInstr &Before,
                        Register Dest, Register Src, unsigned SrcState = 0) {
      if (Dest == Src)
        return;

      unsigned Opcode = IsUpper(Dest) && IsUpper(Src) ? AVM::MOV : AVM::MOV_RR;
      BuildMI(MBB, Before, Before.getDebugLoc(), TII.get(Opcode), Dest)
          .addReg(Src, SrcState);
    };
    auto EmitImmediate = [&](MachineBasicBlock &MBB, MachineInstr &Before,
                             Register Reg, uint16_t Value) {
      if (Value == 0) {
        unsigned Opcode = IsUpper(Reg) ? AVM::XOR : AVM::SUB_RR;
        BuildMI(MBB, Before, Before.getDebugLoc(), TII.get(Opcode), Reg)
            .addReg(Reg, RegState::Undef)
            .addReg(Reg, RegState::Undef);
        return;
      }

      bool IsByte = Value <= 0xff;
      unsigned Opcode = IsUpper(Reg)
                            ? (IsByte ? AVM::LDI8 : AVM::LDI16)
                            : (IsByte ? AVM::COLDLDI8 : AVM::COLDLDI16);
      BuildMI(MBB, Before, Before.getDebugLoc(), TII.get(Opcode), Reg)
          .addImm(Value);
    };
    auto ReplaceWithDisplacedLoad = [&](MachineBasicBlock &MBB,
                                        MachineInstr &MI, unsigned Opcode,
                                        int64_t Displacement) {
      MachineInstrBuilder MIB =
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode));
      MIB.add(MI.getOperand(0));
      MIB.add(MI.getOperand(1));
      MIB.addImm(Displacement).cloneMemRefs(MI);
      MI.eraseFromParent();
    };
    auto ReplaceWithDisplacedStore = [&](MachineBasicBlock &MBB,
                                         MachineInstr &MI, unsigned Opcode,
                                         int64_t Displacement) {
      MachineInstrBuilder MIB =
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode));
      MIB.add(MI.getOperand(0));
      MIB.addImm(Displacement);
      MIB.add(MI.getOperand(1)).cloneMemRefs(MI);
      MI.eraseFromParent();
    };
    auto GetBranchOpcode = [](int64_t Cond) {
      switch (Cond) {
      case AVMCC::EQ:
        return AVM::RELAX_BR_EQ;
      case AVMCC::NE:
        return AVM::RELAX_BR_NE;
      case AVMCC::ULT:
        return AVM::RELAX_BR_ULT;
      case AVMCC::UGE:
        return AVM::RELAX_BR_UGE;
      case AVMCC::SLT:
        return AVM::RELAX_BR_SLT;
      case AVMCC::SGE:
        return AVM::RELAX_BR_SGE;
      default:
        llvm_unreachable("invalid AVM branch condition");
      }
    };
    auto GetCSetOpcode = [](int64_t Cond) {
      switch (Cond) {
      case AVMCC::EQ:
        return AVM::CSET_EQ;
      case AVMCC::NE:
        return AVM::CSET_NE;
      case AVMCC::ULT:
        return AVM::CSET_ULT;
      case AVMCC::UGE:
        return AVM::CSET_UGE;
      case AVMCC::SLT:
        return AVM::CSET_SLT;
      case AVMCC::SGE:
        return AVM::CSET_SGE;
      default:
        llvm_unreachable("invalid AVM CSET condition");
      }
    };
    auto GetCMovOpcode = [](int64_t Cond) {
      switch (Cond) {
      case AVMCC::EQ:
        return AVM::CMOV_EQ;
      case AVMCC::NE:
        return AVM::CMOV_NE;
      case AVMCC::ULT:
        return AVM::CMOV_ULT;
      case AVMCC::UGE:
        return AVM::CMOV_UGE;
      case AVMCC::SLT:
        return AVM::CMOV_SLT;
      case AVMCC::SGE:
        return AVM::CMOV_SGE;
      default:
        llvm_unreachable("invalid AVM CMOV condition");
      }
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
          EmitMove(MBB, MI, MI.getOperand(0).getReg(),
                   MI.getOperand(1).getReg(),
                   MI.getOperand(1).isKill() ? RegState::Kill : 0);
          MI.eraseFromParent();
          Changed = true;
          continue;
        case AVM::COPY32_PSEUDO: {
          Register Dest = MI.getOperand(0).getReg();
          Register Src = MI.getOperand(1).getReg();
          if (Dest == Src) {
            MI.eraseFromParent();
            Changed = true;
            continue;
          }
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
          EmitImmediate(MBB, MI, MI.getOperand(0).getReg(),
                        static_cast<uint8_t>(MI.getOperand(1).getImm()));
          MI.eraseFromParent();
          Changed = true;
          continue;
        case AVM::LDI16_PSEUDO:
          EmitImmediate(MBB, MI, MI.getOperand(0).getReg(),
                        static_cast<uint16_t>(MI.getOperand(1).getImm()));
          MI.eraseFromParent();
          Changed = true;
          continue;
        case AVM::LDI32_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
          Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
          EmitImmediate(MBB, MI, Lo,
                        static_cast<uint16_t>(MI.getOperand(1).getImm()));
          EmitImmediate(MBB, MI, Hi,
                        static_cast<uint16_t>(MI.getOperand(2).getImm()));
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::ZEXT16_32_PSEUDO:
        case AVM::SEXT16_32_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register Src = MI.getOperand(1).getReg();
          Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
          Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
          EmitMove(MBB, MI, Lo, Src);
          if (MI.getOpcode() == AVM::ZEXT16_32_PSEUDO) {
            EmitImmediate(MBB, MI, Hi, 0);
          } else {
            assert(IsUpper(Hi) && "signed extension requires an upper pair");
            EmitMove(MBB, MI, Hi, Src,
                     MI.getOperand(1).isKill() ? RegState::Kill : 0);
            BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::ASR16I), Hi)
                .addReg(Hi)
                .addImm(15);
          }
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::BUILD_HI16_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register Src = MI.getOperand(1).getReg();
          Register DestLo = TRI.getSubReg(Dest, AVM::sub_lo16);
          Register DestHi = TRI.getSubReg(Dest, AVM::sub_hi16);
          EmitMove(MBB, MI, DestHi, Src,
                   MI.getOperand(1).isKill() ? RegState::Kill : 0);
          EmitImmediate(MBB, MI, DestLo, 0);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::SHL32_16_PSEUDO:
        case AVM::SRL32_16_PSEUDO:
        case AVM::SRA32_16_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register Src = MI.getOperand(1).getReg();
          Register DestLo = TRI.getSubReg(Dest, AVM::sub_lo16);
          Register DestHi = TRI.getSubReg(Dest, AVM::sub_hi16);
          Register SrcLo = TRI.getSubReg(Src, AVM::sub_lo16);
          Register SrcHi = TRI.getSubReg(Src, AVM::sub_hi16);
          if (MI.getOpcode() == AVM::SHL32_16_PSEUDO) {
            EmitMove(MBB, MI, DestHi, SrcLo);
            EmitImmediate(MBB, MI, DestLo, 0);
          } else if (MI.getOpcode() == AVM::SRL32_16_PSEUDO) {
            EmitMove(MBB, MI, DestLo, SrcHi);
            EmitImmediate(MBB, MI, DestHi, 0);
          } else {
            EmitMove(MBB, MI, DestLo, SrcHi);
            EmitMove(MBB, MI, DestHi, SrcHi);
            BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::ASR16I), DestHi)
                .addReg(DestHi)
                .addImm(15);
          }
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::SEXT24_32_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::SEXT8), Hi)
              .addReg(Hi);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::PROG_ADDR_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
          Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
          MachineOperand LoAddress = MI.getOperand(1);
          MachineOperand HiAddress = MI.getOperand(1);
          LoAddress.setTargetFlags(AVMII::MO_LO16);
          HiAddress.setTargetFlags(AVMII::MO_HI8);
          BuildMI(MBB, MI, MI.getDebugLoc(),
                  TII.get(IsUpper(Lo) ? AVM::LDI16 : AVM::COLDLDI16), Lo)
              .add(LoAddress);
          BuildMI(MBB, MI, MI.getDebugLoc(),
                  TII.get(IsUpper(Hi) ? AVM::LDI8 : AVM::COLDLDI8), Hi)
              .add(HiAddress);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
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
        case AVM::ADD32_PSEUDO:
          NewOpcode = AVM::ADD32;
          break;
        case AVM::SUB32_PSEUDO:
          NewOpcode = AVM::SUB32;
          break;
        case AVM::AND32_PSEUDO:
        case AVM::OR32_PSEUDO:
        case AVM::XOR32_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register RHS = MI.getOperand(2).getReg();
          Register DestLo = TRI.getSubReg(Dest, AVM::sub_lo16);
          Register DestHi = TRI.getSubReg(Dest, AVM::sub_hi16);
          Register RHSLo = TRI.getSubReg(RHS, AVM::sub_lo16);
          Register RHSHi = TRI.getSubReg(RHS, AVM::sub_hi16);
          unsigned CompactOpcode =
              MI.getOpcode() == AVM::AND32_PSEUDO  ? AVM::AND
              : MI.getOpcode() == AVM::OR32_PSEUDO ? AVM::OR
                                                   : AVM::XOR;
          unsigned FullOpcode =
              MI.getOpcode() == AVM::AND32_PSEUDO  ? AVM::AND_RR
              : MI.getOpcode() == AVM::OR32_PSEUDO ? AVM::OR_RR
                                                   : AVM::XOR_RR;
          auto EmitWord = [&](Register Dst, Register Src) {
            unsigned Opcode =
                IsUpper(Dst) && IsUpper(Src) ? CompactOpcode : FullOpcode;
            BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode), Dst)
                .addReg(Dst)
                .addReg(Src);
          };
          EmitWord(DestLo, RHSLo);
          EmitWord(DestHi, RHSHi);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::PROG_ADD_PSEUDO: {
          NewOpcode = AVM::ADD32;
          break;
        }
        case AVM::PROG_CANON_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::ZEXT8), Hi)
              .addReg(Hi);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::BSWAP32_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
          Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::BSWAP16), Lo)
              .addReg(Lo);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::BSWAP16), Hi)
              .addReg(Hi);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::XOR_RR), Lo)
              .addReg(Lo)
              .addReg(Hi);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::XOR_RR), Hi)
              .addReg(Hi)
              .addReg(Lo);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::XOR_RR), Lo)
              .addReg(Lo)
              .addReg(Hi);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::INC16_PSEUDO:
          NewOpcode = AVM::INC16;
          break;
        case AVM::DEC16_PSEUDO:
          NewOpcode = AVM::DEC16;
          break;
        case AVM::ADDIS8_PSEUDO:
          NewOpcode = IsUpper(MI.getOperand(0).getReg()) ? AVM::ADDIS8
                                                         : AVM::COLDADDIS8;
          break;
        case AVM::MUL8_PSEUDO:
          NewOpcode = AVM::MUL8;
          break;
        case AVM::MULU8W_PSEUDO:
          NewOpcode = AVM::MULU8W;
          break;
        case AVM::MULS8W_PSEUDO:
          NewOpcode = AVM::MULS8W;
          break;
        case AVM::MULSU8W_PSEUDO:
          NewOpcode = AVM::MULSU8W;
          break;
        case AVM::MUL16_PSEUDO:
          NewOpcode = AVM::MUL16;
          break;
        case AVM::UDIV16_PSEUDO:
          NewOpcode = AVM::UDIV16;
          break;
        case AVM::UREM16_PSEUDO:
          NewOpcode = AVM::UREM16;
          break;
        case AVM::SDIV16_PSEUDO:
          NewOpcode = AVM::SDIV16;
          break;
        case AVM::SREM16_PSEUDO:
          NewOpcode = AVM::SREM16;
          break;
        case AVM::CMP16_PSEUDO:
          NewOpcode = PreferCompact(IsUpper(MI.getOperand(0).getReg()) &&
                                        IsUpper(MI.getOperand(1).getReg()),
                                    AVM::CMP, AVM::AVMCostKind::CmpUpper,
                                    AVM::CMP_RR, AVM::AVMCostKind::CmpFull);
          break;
        case AVM::CMP32_PSEUDO:
          NewOpcode = AVM::CMP32;
          break;
        case AVM::CMPIS8_PSEUDO:
          NewOpcode = IsUpper(MI.getOperand(0).getReg()) ? AVM::CMPIS8
                                                         : AVM::COLDCMPIS8;
          break;
        case AVM::TST8_PSEUDO:
          NewOpcode = AVM::TST8;
          break;
        case AVM::TST16_PSEUDO:
          NewOpcode = AVM::TST16;
          break;
        case AVM::CSET_PSEUDO:
          NewOpcode = GetCSetOpcode(MI.getOperand(1).getImm());
          MI.removeOperand(1);
          break;
        case AVM::BR_CC_PSEUDO:
          NewOpcode = GetBranchOpcode(MI.getOperand(1).getImm());
          MI.removeOperand(1);
          break;
        case AVM::JMP_PSEUDO:
          NewOpcode = AVM::RELAX_JMP;
          break;
        case AVM::CMOV16_PSEUDO: {
          unsigned Opcode = GetCMovOpcode(MI.getOperand(3).getImm());
          Register Dest = MI.getOperand(0).getReg();
          const MachineOperand &True = MI.getOperand(1);
          const MachineOperand &False = MI.getOperand(2);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode), Dest)
              .addReg(False.getReg(), getKillRegState(False.isKill()))
              .addReg(True.getReg(), getKillRegState(True.isKill()));
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::CMOV32_PSEUDO: {
          unsigned Opcode = GetCMovOpcode(MI.getOperand(3).getImm());
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register True = MI.getOperand(1).getReg();
          Register DestLo = TRI.getSubReg(Dest, AVM::sub_lo16);
          Register DestHi = TRI.getSubReg(Dest, AVM::sub_hi16);
          Register TrueLo = TRI.getSubReg(True, AVM::sub_lo16);
          Register TrueHi = TRI.getSubReg(True, AVM::sub_hi16);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode), DestLo)
              .addReg(DestLo)
              .addReg(TrueLo);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode), DestHi)
              .addReg(DestHi)
              .addReg(TrueHi);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::SHL16_SMALL_PSEUDO: {
          Register Dest = MI.getOperand(0).getReg();
          unsigned Count = MI.getOperand(2).getImm();
          assert(Count >= 1 && Count <= 3 && "invalid small SHL count");
          unsigned Opcode = IsUpper(Dest) ? AVM::ADD : AVM::LSL16_1;
          for (unsigned I = 0; I != Count; ++I) {
            MachineInstrBuilder MIB =
                BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode), Dest)
                    .addReg(Dest);
            if (Opcode == AVM::ADD)
              MIB.addReg(Dest);
          }
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::LSR16_1_PSEUDO:
          NewOpcode = AVM::LSR16_1;
          break;
        case AVM::ASR16_1_PSEUDO:
          NewOpcode = AVM::ASR16_1;
          break;
        case AVM::LSR32_1_PSEUDO:
          NewOpcode = AVM::LSR32_1;
          break;
        case AVM::SRA32_1_PSEUDO:
          NewOpcode = AVM::ASR32_1;
          break;
        case AVM::LSL16I_PSEUDO:
          NewOpcode = AVM::LSL16I;
          break;
        case AVM::LSR16I_PSEUDO:
          NewOpcode = AVM::LSR16I;
          break;
        case AVM::ASR16I_PSEUDO:
          NewOpcode = AVM::ASR16I;
          break;
        case AVM::SHL16V_PSEUDO:
          NewOpcode = AVM::SHL16V;
          break;
        case AVM::LSR16V_PSEUDO:
          NewOpcode = AVM::LSR16V;
          break;
        case AVM::ASR16V_PSEUDO:
          NewOpcode = AVM::ASR16V;
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
          if (!IsUpper(Addr)) {
            ReplaceWithDisplacedLoad(MBB, MI, AVM::DPLD8U, 0);
            Changed = true;
            continue;
          }
          NewOpcode = IsUpper(Dest) ? AVM::LD8U : AVM::F5LD8U;
          break;
        }
        case AVM::LOAD16_PSEUDO: {
          Register Dest = MI.getOperand(0).getReg();
          Register Addr = MI.getOperand(1).getReg();
          if (!IsUpper(Addr)) {
            ReplaceWithDisplacedLoad(MBB, MI, AVM::DPLD16, 0);
            Changed = true;
            continue;
          }
          NewOpcode = IsUpper(Dest) ? AVM::LD16 : AVM::F5LD16;
          break;
        }
        case AVM::STORE8_PSEUDO: {
          Register Addr = MI.getOperand(0).getReg();
          Register Src = MI.getOperand(1).getReg();
          if (!IsUpper(Addr)) {
            ReplaceWithDisplacedStore(MBB, MI, AVM::DPST8, 0);
            Changed = true;
            continue;
          }
          NewOpcode = IsUpper(Src) ? AVM::ST8 : AVM::F3ST8;
          break;
        }
        case AVM::STORE16_PSEUDO: {
          Register Addr = MI.getOperand(0).getReg();
          Register Src = MI.getOperand(1).getReg();
          if (!IsUpper(Addr)) {
            ReplaceWithDisplacedStore(MBB, MI, AVM::DPST16, 0);
            Changed = true;
            continue;
          }
          NewOpcode = IsUpper(Src) ? AVM::ST16 : AVM::F5ST16;
          break;
        }
        case AVM::LOAD8U_DISP_PSEUDO:
          assert(MI.getOperand(2).isImm() && MI.getOperand(2).getImm() >= -32 &&
                 MI.getOperand(2).getImm() <= 223);
          NewOpcode = AVM::DPLD8U;
          break;
        case AVM::LOAD16_DISP_PSEUDO:
          assert(MI.getOperand(2).isImm() && MI.getOperand(2).getImm() >= -32 &&
                 MI.getOperand(2).getImm() <= 223);
          NewOpcode = AVM::DPLD16;
          break;
        case AVM::STORE8_DISP_PSEUDO:
          assert(MI.getOperand(1).isImm() && MI.getOperand(1).getImm() >= -32 &&
                 MI.getOperand(1).getImm() <= 223);
          NewOpcode = AVM::DPST8;
          break;
        case AVM::STORE16_DISP_PSEUDO:
          assert(MI.getOperand(1).isImm() && MI.getOperand(1).getImm() >= -32 &&
                 MI.getOperand(1).getImm() <= 223);
          NewOpcode = AVM::DPST16;
          break;
        case AVM::LOAD32_PSEUDO:
          NewOpcode = AVM::LD32;
          break;
        case AVM::STORE32_PSEUDO:
          NewOpcode = AVM::ST32;
          break;
        case AVM::LOAD24_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Dest = MI.getOperand(0).getReg();
          Register Addr = MI.getOperand(1).getReg();
          Register AddrIn = MI.getOperand(2).getReg();
          Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
          Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::GPLD16_POST), Lo)
              .addDef(Addr)
              .addReg(AddrIn);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::DPLD8U), Hi)
              .addReg(Addr)
              .addImm(0)
              .cloneMemRefs(MI);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::STORE24_PSEUDO: {
          const AVMRegisterInfo &TRI = TII.getRegisterInfo();
          Register Addr = MI.getOperand(0).getReg();
          Register AddrIn = MI.getOperand(1).getReg();
          Register Src = MI.getOperand(2).getReg();
          Register Lo = TRI.getSubReg(Src, AVM::sub_lo16);
          Register Hi = TRI.getSubReg(Src, AVM::sub_hi16);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::GPST16_POST), Addr)
              .addReg(AddrIn)
              .addReg(Lo);
          BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVM::DPST8))
              .addReg(Addr)
              .addImm(0)
              .addReg(Hi)
              .cloneMemRefs(MI);
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
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
        case AVM::LOAD8U_PRE_PSEUDO:
        case AVM::LOAD16_PRE_PSEUDO: {
          bool IsByte = MI.getOpcode() == AVM::LOAD8U_PRE_PSEUDO;
          Register Dest = MI.getOperand(0).getReg();
          Register Addr = MI.getOperand(1).getReg();
          Register AddrIn = MI.getOperand(2).getReg();
          MachineInstrBuilder Adjust =
              BuildMI(MBB, MI, MI.getDebugLoc(),
                      TII.get(IsByte ? AVM::DEC16
                                     : (IsUpper(Addr) ? AVM::ADDIS8
                                                      : AVM::COLDADDIS8)),
                      Addr)
                  .addReg(AddrIn, getKillRegState(MI.getOperand(2).isKill()));
          if (!IsByte)
            Adjust.addImm(-2);
          if (!IsUpper(Addr)) {
            BuildMI(MBB, MI, MI.getDebugLoc(),
                    TII.get(IsByte ? AVM::DPLD8U : AVM::DPLD16), Dest)
                .addReg(Addr)
                .addImm(0)
                .cloneMemRefs(MI);
          } else {
            unsigned LoadOpcode = IsUpper(Dest)
                                      ? (IsByte ? AVM::LD8U : AVM::LD16)
                                      : (IsByte ? AVM::F5LD8U : AVM::F5LD16);
            BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(LoadOpcode), Dest)
                .addReg(Addr)
                .cloneMemRefs(MI);
          }
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
        case AVM::STORE8_PRE_PSEUDO:
        case AVM::STORE16_PRE_PSEUDO: {
          bool IsByte = MI.getOpcode() == AVM::STORE8_PRE_PSEUDO;
          Register Addr = MI.getOperand(0).getReg();
          Register AddrIn = MI.getOperand(1).getReg();
          Register Src = MI.getOperand(2).getReg();
          MachineInstrBuilder Adjust =
              BuildMI(MBB, MI, MI.getDebugLoc(),
                      TII.get(IsByte ? AVM::DEC16
                                     : (IsUpper(Addr) ? AVM::ADDIS8
                                                      : AVM::COLDADDIS8)),
                      Addr)
                  .addReg(AddrIn, getKillRegState(MI.getOperand(1).isKill()));
          if (!IsByte)
            Adjust.addImm(-2);
          if (!IsUpper(Addr)) {
            BuildMI(MBB, MI, MI.getDebugLoc(),
                    TII.get(IsByte ? AVM::DPST8 : AVM::DPST16))
                .addReg(Addr)
                .addImm(0)
                .addReg(Src, getKillRegState(MI.getOperand(2).isKill()))
                .cloneMemRefs(MI);
          } else {
            unsigned StoreOpcode = IsUpper(Src)
                                       ? (IsByte ? AVM::ST8 : AVM::ST16)
                                       : (IsByte ? AVM::F3ST8 : AVM::F5ST16);
            BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(StoreOpcode))
                .addReg(Addr)
                .addReg(Src, getKillRegState(MI.getOperand(2).isKill()))
                .cloneMemRefs(MI);
          }
          MI.eraseFromParent();
          Changed = true;
          continue;
        }
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
        case AVM::PLOAD8U_PSEUDO:
          NewOpcode = AVM::LDP8U;
          break;
        case AVM::PLOAD8S_PSEUDO:
          NewOpcode = AVM::LDP8S;
          break;
        case AVM::PLOAD16_PSEUDO:
          NewOpcode = AVM::LDP16;
          break;
        case AVM::PLOAD24_PSEUDO:
          NewOpcode = AVM::LDP24;
          break;
        case AVM::PLOAD32_PSEUDO:
          NewOpcode = AVM::LDP32;
          break;
        case AVM::PLOAD8U_POST_PSEUDO:
          NewOpcode = AVM::LDP8U_POST;
          break;
        case AVM::PLOAD16_POST_PSEUDO:
          NewOpcode = AVM::LDP16_POST;
          break;
        case AVM::PLOAD24_POST_PSEUDO:
          NewOpcode = AVM::LDP24_POST;
          break;
        case AVM::PLOAD32_POST_PSEUDO:
          NewOpcode = AVM::LDP32_POST;
          break;
        case AVM::FADD_PSEUDO:
          NewOpcode = AVM::FADD;
          break;
        case AVM::FSUB_PSEUDO:
          NewOpcode = AVM::FSUB;
          break;
        case AVM::FMUL_PSEUDO:
          NewOpcode = AVM::FMUL;
          break;
        case AVM::FDIV_PSEUDO:
          NewOpcode = AVM::FDIV;
          break;
        case AVM::FMIN_PSEUDO:
          NewOpcode = AVM::FMIN;
          break;
        case AVM::FMAX_PSEUDO:
          NewOpcode = AVM::FMAX;
          break;
        case AVM::FNEG_PSEUDO:
          NewOpcode = AVM::FNEG;
          break;
        case AVM::FABS_PSEUDO:
          NewOpcode = AVM::FABS;
          break;
        case AVM::FSQRT_PSEUDO:
          NewOpcode = AVM::FSQRT;
          break;
        case AVM::S16TOF_PSEUDO:
          NewOpcode = AVM::S16TOF;
          break;
        case AVM::U16TOF_PSEUDO:
          NewOpcode = AVM::U16TOF;
          break;
        case AVM::S32TOF_PSEUDO:
          NewOpcode = AVM::S32TOF;
          break;
        case AVM::U32TOF_PSEUDO:
          NewOpcode = AVM::U32TOF;
          break;
        case AVM::FTOS16_PSEUDO:
          NewOpcode = AVM::FTOS16;
          break;
        case AVM::FTOU16_PSEUDO:
          NewOpcode = AVM::FTOU16;
          break;
        case AVM::FTOS32_PSEUDO:
          NewOpcode = AVM::FTOS32;
          break;
        case AVM::FTOU32_PSEUDO:
          NewOpcode = AVM::FTOU32;
          break;
        case AVM::FCMP_PSEUDO:
          NewOpcode = AVM::FCMP;
          break;
        case AVM::FCLASS_PSEUDO:
          NewOpcode = AVM::FCLASS;
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
