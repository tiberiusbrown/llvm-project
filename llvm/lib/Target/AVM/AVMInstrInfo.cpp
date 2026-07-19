//===-- AVMInstrInfo.cpp - AVM instruction information -------------------===//

#include "AVMInstrInfo.h"
#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "AVMGenInstrInfo.inc"

AVMInstrInfo::AVMInstrInfo(const AVMSubtarget &STI)
    : AVMGenInstrInfo(STI, RI, AVM::ADJCALLSTACKDOWN, AVM::ADJCALLSTACKUP),
      RI() {}

void AVMInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MI,
                               const DebugLoc &DL, Register DestReg,
                               Register SrcReg, bool KillSrc, bool,
                               bool) const {
  if (AVM::GPR16RegClass.contains(DestReg, SrcReg)) {
    BuildMI(MBB, MI, DL, get(AVM::COPY16_PSEUDO), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }
  if (AVM::GPR32RegClass.contains(DestReg, SrcReg)) {
    BuildMI(MBB, MI, DL, get(AVM::COPY32_PSEUDO), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }
  if (SrcReg == AVM::SP && AVM::GPR16RegClass.contains(DestReg)) {
    BuildMI(MBB, MI, DL, get(AVM::GETSP), DestReg);
    return;
  }
  if (DestReg == AVM::SP && AVM::GPR16RegClass.contains(SrcReg)) {
    BuildMI(MBB, MI, DL, get(AVM::SETSP))
        .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }
  report_fatal_error("unsupported AVM physical-register copy");
}

void AVMInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MI,
                                       Register SrcReg, bool IsKill,
                                       int FrameIndex,
                                       const TargetRegisterClass *RC, Register,
                                       MachineInstr::MIFlag Flags) const {
  MachineFunction &MF = *MBB.getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOStore, MFI.getObjectSize(FrameIndex), Align(1));
  unsigned Opcode;
  if (AVM::GPR16RegClass.hasSubClassEq(RC))
    Opcode = AVM::STACK_STORE16_PSEUDO;
  else if (AVM::GPR32RegClass.hasSubClassEq(RC))
    Opcode = AVM::STACK_STORE32_PSEUDO;
  else
    report_fatal_error("unsupported AVM spill register class");
  DebugLoc DL = MI == MBB.end() ? DebugLoc() : MI->getDebugLoc();
  BuildMI(MBB, MI, DL, get(Opcode))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addReg(SrcReg, getKillRegState(IsKill))
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}

void AVMInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        Register DestReg, int FrameIndex,
                                        const TargetRegisterClass *RC, Register,
                                        unsigned SubReg,
                                        MachineInstr::MIFlag Flags) const {
  if (SubReg)
    report_fatal_error("AVM subregister reloads are not supported");
  MachineFunction &MF = *MBB.getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIndex), Align(1));
  unsigned Opcode;
  if (AVM::GPR16RegClass.hasSubClassEq(RC))
    Opcode = AVM::STACK_LOAD16_PSEUDO;
  else if (AVM::GPR32RegClass.hasSubClassEq(RC))
    Opcode = AVM::STACK_LOAD32_PSEUDO;
  else
    report_fatal_error("unsupported AVM reload register class");
  DebugLoc DL = MI == MBB.end() ? DebugLoc() : MI->getDebugLoc();
  BuildMI(MBB, MI, DL, get(Opcode), DestReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}

unsigned AVMInstrInfo::getInstrLatency(const InstrItineraryData *ItinData,
                                       const MachineInstr &MI,
                                       unsigned *PredCost) const {
  using AVM::AVMCostKind;
  auto Fixed = [](AVMCostKind Kind) { return AVM::getFixedCycles(Kind); };
  auto Typical = [](AVMCostKind Kind) {
    return AVM::getCycleRange(Kind).Typical;
  };
  auto NotTaken = [](AVMCostKind Kind) {
    return AVM::getBranchCycles(Kind, false);
  };

  switch (MI.getOpcode()) {
  case AVM::MOV:
    return Fixed(AVMCostKind::MovUpper);
  case AVM::MOV_RR:
    return Fixed(AVMCostKind::MovFull);
  case AVM::MOV32_F2:
    return Fixed(AVMCostKind::Mov32Full);
  case AVM::LDI8:
    return Fixed(AVMCostKind::Ldi8Upper);
  case AVM::COLDLDI8:
    return Fixed(AVMCostKind::Ldi8Lower);
  case AVM::LDI16:
    return Fixed(AVMCostKind::Ldi16Upper);
  case AVM::COLDLDI16:
    return Fixed(AVMCostKind::Ldi16Lower);
  case AVM::ZEXT8:
    return Fixed(AVMCostKind::ZExt8);
  case AVM::SEXT8:
    return Fixed(AVMCostKind::SExt8);
  case AVM::SWAP8:
    return Fixed(AVMCostKind::Swap8);
  case AVM::BSWAP16:
    return Fixed(AVMCostKind::BSwap16);
  case AVM::BOOL:
    return Fixed(AVMCostKind::Bool);
  case AVM::PUSH16:
    return Fixed(AVMCostKind::Push16);
  case AVM::POP16:
    return Fixed(AVMCostKind::Pop16);
  case AVM::ADJSP:
    return Fixed(AVMCostKind::AdjSP);
  case AVM::GETSP:
    return Fixed(AVMCostKind::GetSP);
  case AVM::SETSP:
    return Fixed(AVMCostKind::SetSP);
  case AVM::LEASP:
    return Fixed(AVMCostKind::LeaSP);
  case AVM::ADD:
    return Fixed(AVMCostKind::AddUpper);
  case AVM::ADD_RR:
    return Fixed(AVMCostKind::AddFull);
  case AVM::SUB:
    return Fixed(AVMCostKind::SubUpper);
  case AVM::SUB_RR:
    return Fixed(AVMCostKind::SubFull);
  case AVM::ADDIS8:
    return Fixed(AVMCostKind::AddIS8Upper);
  case AVM::COLDADDIS8:
    return Fixed(AVMCostKind::AddIS8Lower);
  case AVM::INC16:
    return Fixed(AVMCostKind::Inc16);
  case AVM::DEC16:
    return Fixed(AVMCostKind::Dec16);
  case AVM::NEG16:
    return Fixed(AVMCostKind::Neg16);
  case AVM::ADD32:
    return Fixed(AVMCostKind::Add32);
  case AVM::SUB32:
    return Fixed(AVMCostKind::Sub32);
  case AVM::AND:
    return Fixed(AVMCostKind::AndUpper);
  case AVM::AND_RR:
    return Fixed(AVMCostKind::AndFull);
  case AVM::OR:
    return Fixed(AVMCostKind::OrUpper);
  case AVM::OR_RR:
    return Fixed(AVMCostKind::OrFull);
  case AVM::XOR:
    return Fixed(AVMCostKind::XorUpper);
  case AVM::XOR_RR:
    return Fixed(AVMCostKind::XorFull);
  case AVM::NOT16:
    return Fixed(AVMCostKind::Not16);
  case AVM::LSL16_1:
    return Fixed(AVMCostKind::Lsl16One);
  case AVM::LSR16_1:
    return Fixed(AVMCostKind::Lsr16One);
  case AVM::ASR16_1:
    return Fixed(AVMCostKind::Asr16One);
  case AVM::LSR32_1:
    return Fixed(AVMCostKind::Lsr32One);
  case AVM::ASR32_1:
    return Fixed(AVMCostKind::Asr32One);
  case AVM::MUL8:
    return Fixed(AVMCostKind::Mul8);
  case AVM::MULU8W:
    return Fixed(AVMCostKind::MulU8W);
  case AVM::MULS8W:
    return Fixed(AVMCostKind::MulS8W);
  case AVM::MULSU8W:
    return Fixed(AVMCostKind::MulSU8W);
  case AVM::MUL16:
    return Fixed(AVMCostKind::Mul16);
  case AVM::FNEG:
    return Fixed(AVMCostKind::FNeg);
  case AVM::FABS:
    return Fixed(AVMCostKind::FAbs);
  case AVM::CMP:
    return Fixed(AVMCostKind::CmpUpper);
  case AVM::CMP_RR:
    return Fixed(AVMCostKind::CmpFull);
  case AVM::CMP32:
    return Fixed(AVMCostKind::Cmp32);
  case AVM::CMPIS8:
    return Fixed(AVMCostKind::CmpIS8Upper);
  case AVM::COLDCMPIS8:
    return Fixed(AVMCostKind::CmpIS8Lower);
  case AVM::TST8:
    return Fixed(AVMCostKind::Tst8);
  case AVM::TST16:
    return Fixed(AVMCostKind::Tst16);
  case AVM::CSET_EQ:
    return Fixed(AVMCostKind::CSetEq);
  case AVM::CSET_NE:
    return Fixed(AVMCostKind::CSetNe);
  case AVM::CSET_ULT:
    return Fixed(AVMCostKind::CSetUlt);
  case AVM::CSET_UGE:
    return Fixed(AVMCostKind::CSetUge);
  case AVM::CSET_SLT:
    return Fixed(AVMCostKind::CSetSlt);
  case AVM::CSET_SGE:
    return Fixed(AVMCostKind::CSetSge);
  case AVM::LD8U:
    return Fixed(AVMCostKind::Ld8UUpper);
  case AVM::ST8:
    return Fixed(AVMCostKind::St8Upper);
  case AVM::LD16:
    return Fixed(AVMCostKind::Ld16Upper);
  case AVM::ST16:
    return Fixed(AVMCostKind::St16Upper);
  case AVM::F3ST8:
    return Fixed(AVMCostKind::St8Dense);
  case AVM::F5LD8U:
    return Fixed(AVMCostKind::Ld8UDense);
  case AVM::F5LD16:
    return Fixed(AVMCostKind::Ld16Dense);
  case AVM::F5ST16:
    return Fixed(AVMCostKind::St16Dense);
  case AVM::F7LD8U_POST:
    return Fixed(AVMCostKind::Ld8UPostIncDense);
  case AVM::F7LD16_POST:
    return Fixed(AVMCostKind::Ld16PostIncDense);
  case AVM::F6ST8_POST:
    return Fixed(AVMCostKind::St8PostIncDense);
  case AVM::F7ST16_POST:
    return Fixed(AVMCostKind::St16PostIncDense);
  case AVM::GPLD8U:
    return Fixed(AVMCostKind::Ld8UGeneral);
  case AVM::GPLD16:
    return Fixed(AVMCostKind::Ld16General);
  case AVM::GPST8:
    return Fixed(AVMCostKind::St8General);
  case AVM::GPST16:
    return Fixed(AVMCostKind::St16General);
  case AVM::GPLD8U_POST:
    return Fixed(AVMCostKind::Ld8UPostIncGeneral);
  case AVM::GPLD16_POST:
    return Fixed(AVMCostKind::Ld16PostIncGeneral);
  case AVM::GPST8_POST:
    return Fixed(AVMCostKind::St8PostIncGeneral);
  case AVM::GPST16_POST:
    return Fixed(AVMCostKind::St16PostIncGeneral);
  case AVM::LDSP8U_COMPACT:
    return Fixed(AVMCostKind::LdSp8UShort);
  case AVM::STSP8_COMPACT:
    return Fixed(AVMCostKind::StSp8Short);
  case AVM::LDSP16_COMPACT:
    return Fixed(AVMCostKind::LdSp16Short);
  case AVM::STSP16_COMPACT:
    return Fixed(AVMCostKind::StSp16Short);
  case AVM::LDSP8U:
    return Fixed(AVMCostKind::LdSp8UCold);
  case AVM::LDSP8S:
    return Fixed(AVMCostKind::LdSp8SCold);
  case AVM::STSP8:
    return Fixed(AVMCostKind::StSp8Cold);
  case AVM::LDSP16:
    return Fixed(AVMCostKind::LdSp16Cold);
  case AVM::STSP16:
    return Fixed(AVMCostKind::StSp16Cold);
  case AVM::LDM8U:
    return Fixed(AVMCostKind::Ldm8U);
  case AVM::STM8:
    return Fixed(AVMCostKind::Stm8);
  case AVM::LDM16:
    return Fixed(AVMCostKind::Ldm16);
  case AVM::STM16:
    return Fixed(AVMCostKind::Stm16);
  case AVM::LD32:
    return Fixed(AVMCostKind::Ld32);
  case AVM::ST32:
    return Fixed(AVMCostKind::St32);
  case AVM::LDP8U:
    return Fixed(AVMCostKind::Ldp8U);
  case AVM::LDP8S:
    return Fixed(AVMCostKind::Ldp8S);
  case AVM::LDP16:
    return Fixed(AVMCostKind::Ldp16);
  case AVM::LDP24:
    return Fixed(AVMCostKind::Ldp24);
  case AVM::LDP32:
    return Fixed(AVMCostKind::Ldp32);
  case AVM::LDP8U_POST:
    return Fixed(AVMCostKind::Ldp8UPostInc);
  case AVM::LDP16_POST:
    return Fixed(AVMCostKind::Ldp16PostInc);
  case AVM::LDP24_POST:
    return Fixed(AVMCostKind::Ldp24PostInc);
  case AVM::LDP32_POST:
    return Fixed(AVMCostKind::Ldp32PostInc);
  case AVM::JMP8:
    return Fixed(AVMCostKind::Jmp8);
  case AVM::CALL8:
    return Fixed(AVMCostKind::Call8);
  case AVM::JMP16:
    return Fixed(AVMCostKind::Jmp16);
  case AVM::CALL16:
    return Fixed(AVMCostKind::Call16);
  case AVM::JMPF:
    return Fixed(AVMCostKind::JmpFar);
  case AVM::CALLF:
    return Fixed(AVMCostKind::CallFar);
  case AVM::JMPP:
    return Fixed(AVMCostKind::JmpPtr);
  case AVM::CALLP:
    return Fixed(AVMCostKind::CallPtr);
  case AVM::RET:
    return Fixed(AVMCostKind::Ret);
  case AVM::UDIV16:
    return Typical(AVMCostKind::UDiv16);
  case AVM::UREM16:
    return Typical(AVMCostKind::URem16);
  case AVM::SDIV16:
    return Typical(AVMCostKind::SDiv16);
  case AVM::SREM16:
    return Typical(AVMCostKind::SRem16);
  case AVM::FADD:
    return Typical(AVMCostKind::FAdd);
  case AVM::FSUB:
    return Typical(AVMCostKind::FSub);
  case AVM::FMUL:
    return Typical(AVMCostKind::FMul);
  case AVM::FDIV:
    return Typical(AVMCostKind::FDiv);
  case AVM::FMIN:
    return Typical(AVMCostKind::FMin);
  case AVM::FMAX:
    return Typical(AVMCostKind::FMax);
  case AVM::FSQRT:
    return Typical(AVMCostKind::FSqrt);
  case AVM::FTRUNC:
    return Typical(AVMCostKind::FTrunc);
  case AVM::FFLOOR:
    return Typical(AVMCostKind::FFloor);
  case AVM::FCEIL:
    return Typical(AVMCostKind::FCeil);
  case AVM::FROUND:
    return Typical(AVMCostKind::FRound);
  case AVM::S16TOF:
    return Typical(AVMCostKind::S16ToF);
  case AVM::U16TOF:
    return Typical(AVMCostKind::U16ToF);
  case AVM::FTOS16:
    return Typical(AVMCostKind::FToS16);
  case AVM::FTOU16:
    return Typical(AVMCostKind::FToU16);
  case AVM::S32TOF:
    return Typical(AVMCostKind::S32ToF);
  case AVM::U32TOF:
    return Typical(AVMCostKind::U32ToF);
  case AVM::FTOS32:
    return Typical(AVMCostKind::FToS32);
  case AVM::FTOU32:
    return Typical(AVMCostKind::FToU32);
  case AVM::FCMP:
    return Typical(AVMCostKind::FCmp);
  case AVM::FCLASS:
    return Typical(AVMCostKind::FClass);
  case AVM::CMOV_EQ:
    return NotTaken(AVMCostKind::CmovEq);
  case AVM::CMOV_NE:
    return NotTaken(AVMCostKind::CmovNe);
  case AVM::CMOV_ULT:
    return NotTaken(AVMCostKind::CmovUlt);
  case AVM::CMOV_UGE:
    return NotTaken(AVMCostKind::CmovUge);
  case AVM::CMOV_SLT:
    return NotTaken(AVMCostKind::CmovSlt);
  case AVM::CMOV_SGE:
    return NotTaken(AVMCostKind::CmovSge);
  case AVM::BREQ8:
    return NotTaken(AVMCostKind::BrEq8);
  case AVM::BRNE8:
    return NotTaken(AVMCostKind::BrNe8);
  case AVM::BRULT8:
    return NotTaken(AVMCostKind::BrUlt8);
  case AVM::BRUGE8:
    return NotTaken(AVMCostKind::BrUge8);
  case AVM::BRSLT8:
    return NotTaken(AVMCostKind::BrSlt8);
  case AVM::BRSGE8:
    return NotTaken(AVMCostKind::BrSge8);
  case AVM::BREQ16:
    return NotTaken(AVMCostKind::BrEq16);
  case AVM::BRNE16:
    return NotTaken(AVMCostKind::BrNe16);
  case AVM::BRULT16:
    return NotTaken(AVMCostKind::BrUlt16);
  case AVM::BRUGE16:
    return NotTaken(AVMCostKind::BrUge16);
  case AVM::BRSLT16:
    return NotTaken(AVMCostKind::BrSlt16);
  case AVM::BRSGE16:
    return NotTaken(AVMCostKind::BrSge16);
  case AVM::SYS:
    if (!MI.getOperand(0).isImm())
      break;
    switch (MI.getOperand(0).getImm()) {
    case 0:
      return Fixed(AVMCostKind::SysDebugPutc);
    case 1:
      return Fixed(AVMCostKind::SysDebugBreak);
    case 2:
      return Fixed(AVMCostKind::SysMillis);
    case 3:
      return Fixed(AVMCostKind::SysMillis32);
    case 4:
      return Typical(AVMCostKind::SysSinf);
    case 5:
      return Typical(AVMCostKind::SysCosf);
    case 6:
      return Typical(AVMCostKind::SysAtan2f);
    case 7:
      return Typical(AVMCostKind::SysTanf);
    case 8:
      return Typical(AVMCostKind::SysExpf);
    case 9:
      return Typical(AVMCostKind::SysLogf);
    case 10:
      return Typical(AVMCostKind::SysLog2f);
    case 11:
      return Typical(AVMCostKind::SysLog10f);
    case 12:
      return Typical(AVMCostKind::SysPowf);
    case 13:
      return Typical(AVMCostKind::SysHypotf);
    case 14:
      return Typical(AVMCostKind::SysFmodf);
    }
    break;
  default:
    break;
  }

  // Shift latency is count-dependent and will be wired up with shift lowering.
  return AVMGenInstrInfo::getInstrLatency(ItinData, MI, PredCost);
}
