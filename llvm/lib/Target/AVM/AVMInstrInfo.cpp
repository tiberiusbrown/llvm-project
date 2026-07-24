//===-- AVMInstrInfo.cpp - AVM instruction information -------------------===//

#include "AVMInstrInfo.h"
#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMSubtarget.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"

#include <limits>

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "AVMGenInstrInfo.inc"

AVMInstrInfo::AVMInstrInfo(const AVMSubtarget &STI)
    : AVMGenInstrInfo(STI, RI, AVM::ADJCALLSTACKDOWN, AVM::ADJCALLSTACKUP),
      RI() {}

bool AVMInstrInfo::isReMaterializableImpl(const MachineInstr &MI) const {
  if ((MI.getOpcode() == AVM::LDI8_PSEUDO ||
       MI.getOpcode() == AVM::LDI16_PSEUDO ||
       MI.getOpcode() == AVM::LDI32_PSEUDO) &&
      !MI.getFlag(MachineInstr::NoMerge))
    return false;
  return TargetInstrInfo::isReMaterializableImpl(MI);
}

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
  if (AVM::GPR16RegClass.contains(DestReg) &&
      AVM::GPR32RegClass.contains(SrcReg)) {
    Register SrcLo = RI.getSubReg(SrcReg, AVM::sub_lo16);
    BuildMI(MBB, MI, DL, get(AVM::COPY16_PSEUDO), DestReg)
        .addReg(SrcLo, getKillRegState(KillSrc));
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
  report_fatal_error(
    Twine("unsupported AVM physical-register copy: ") +
    RI.getName(SrcReg) + " -> " + RI.getName(DestReg));
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

namespace {
bool getAVMBranchCondition(unsigned Opcode, AVMCC::CondCode &Cond) {
  switch (Opcode) {
  case AVM::BR_CC_PSEUDO:
    return false;
  case AVM::RELAX_BR_EQ:
  case AVM::BREQ8:
  case AVM::BREQ16:
    Cond = AVMCC::EQ;
    return true;
  case AVM::RELAX_BR_NE:
  case AVM::BRNE8:
  case AVM::BRNE16:
    Cond = AVMCC::NE;
    return true;
  case AVM::RELAX_BR_ULT:
  case AVM::BRULT8:
  case AVM::BRULT16:
    Cond = AVMCC::ULT;
    return true;
  case AVM::RELAX_BR_UGE:
  case AVM::BRUGE8:
  case AVM::BRUGE16:
    Cond = AVMCC::UGE;
    return true;
  case AVM::RELAX_BR_SLT:
  case AVM::BRSLT8:
  case AVM::BRSLT16:
    Cond = AVMCC::SLT;
    return true;
  case AVM::RELAX_BR_SGE:
  case AVM::BRSGE8:
  case AVM::BRSGE16:
    Cond = AVMCC::SGE;
    return true;
  default:
    return false;
  }
}

bool isAVMConditionalBranch(const MachineInstr &MI) {
  if (MI.getOpcode() == AVM::BR_CC_PSEUDO)
    return true;
  AVMCC::CondCode Cond;
  return getAVMBranchCondition(MI.getOpcode(), Cond);
}

bool isAVMUnconditionalBranch(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case AVM::JMP_PSEUDO:
  case AVM::RELAX_JMP:
  case AVM::JMP8:
  case AVM::JMP16:
  case AVM::JMPF:
    return true;
  default:
    return false;
  }
}

unsigned getSemanticBranchSize(const MachineInstr &MI) {
  if (MI.getOpcode() == AVM::BR_CC_PSEUDO)
    return 6;
  if (MI.getOpcode() == AVM::JMP_PSEUDO)
    return 4;
  return MI.getDesc().getSize();
}

unsigned countPredicableCopies(MachineBasicBlock &MBB) {
  unsigned Count = 0;
  for (MachineInstr &MI : MBB) {
    if (MI.isDebugInstr() || MI.isBranch())
      continue;
    if (MI.getOpcode() == AVM::COPY16_PSEUDO)
      ++Count;
    else if (MI.getOpcode() == AVM::COPY32_PSEUDO)
      Count += 2;
    else
      return std::numeric_limits<unsigned>::max();
    if (Count > 2)
      return Count;
  }
  return Count;
}
} // namespace

bool AVMInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                 MachineBasicBlock *&TBB,
                                 MachineBasicBlock *&FBB,
                                 SmallVectorImpl<MachineOperand> &Cond,
                                 bool) const {
  TBB = FBB = nullptr;
  Cond.clear();

  SmallVector<MachineInstr *, 2> Branches;
  for (MachineInstr &MI : llvm::reverse(MBB)) {
    if (MI.isDebugInstr())
      continue;
    if (!MI.isTerminator())
      break;
    if (!isAVMConditionalBranch(MI) && !isAVMUnconditionalBranch(MI))
      return true;
    Branches.push_back(&MI);
    if (Branches.size() == 2)
      break;
  }
  if (Branches.empty())
    return false;

  MachineInstr *Last = Branches[0];
  if (isAVMConditionalBranch(*Last)) {
    if (!Last->getOperand(0).isMBB())
      return true;
    TBB = Last->getOperand(0).getMBB();
    if (Last->getOpcode() == AVM::BR_CC_PSEUDO)
      Cond.push_back(Last->getOperand(1));
    else {
      AVMCC::CondCode CC;
      if (!getAVMBranchCondition(Last->getOpcode(), CC))
        return true;
      Cond.push_back(MachineOperand::CreateImm(CC));
    }
    return false;
  }

  if (!Last->getOperand(0).isMBB())
    return true;
  if (Branches.size() == 1) {
    TBB = Last->getOperand(0).getMBB();
    return false;
  }

  MachineInstr *First = Branches[1];
  if (!isAVMConditionalBranch(*First) || !First->getOperand(0).isMBB())
    return true;
  TBB = First->getOperand(0).getMBB();
  FBB = Last->getOperand(0).getMBB();
  if (First->getOpcode() == AVM::BR_CC_PSEUDO)
    Cond.push_back(First->getOperand(1));
  else {
    AVMCC::CondCode CC;
    if (!getAVMBranchCondition(First->getOpcode(), CC))
      return true;
    Cond.push_back(MachineOperand::CreateImm(CC));
  }
  return false;
}

unsigned AVMInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                    int *BytesRemoved) const {
  unsigned Removed = 0;
  int Bytes = 0;
  while (!MBB.empty() && Removed != 2) {
    MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
    if (I == MBB.end() ||
        (!isAVMConditionalBranch(*I) && !isAVMUnconditionalBranch(*I)))
      break;
    Bytes += getSemanticBranchSize(*I);
    I->eraseFromParent();
    ++Removed;
  }
  if (BytesRemoved)
    *BytesRemoved = Bytes;
  return Removed;
}

unsigned AVMInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                    MachineBasicBlock *TBB,
                                    MachineBasicBlock *FBB,
                                    ArrayRef<MachineOperand> Cond,
                                    const DebugLoc &DL, int *BytesAdded) const {
  assert(TBB && "AVM branch target is required");
  assert((Cond.empty() || Cond.size() == 1) && "invalid AVM branch condition");
  unsigned Added = 0;
  int Bytes = 0;
  if (Cond.empty()) {
    BuildMI(&MBB, DL, get(AVM::JMP_PSEUDO)).addMBB(TBB);
    Added = 1;
    Bytes = 4;
  } else {
    assert(Cond[0].isImm() && "AVM condition must be an immediate");
    BuildMI(&MBB, DL, get(AVM::BR_CC_PSEUDO))
        .addMBB(TBB)
        .addImm(Cond[0].getImm());
    Added = 1;
    Bytes = 6;
    if (FBB) {
      BuildMI(&MBB, DL, get(AVM::JMP_PSEUDO)).addMBB(FBB);
      ++Added;
      Bytes += 4;
    }
  }
  if (BytesAdded)
    *BytesAdded = Bytes;
  return Added;
}

bool AVMInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  if (Cond.size() != 1 || !Cond[0].isImm())
    return true;
  switch (Cond[0].getImm()) {
  case AVMCC::EQ:
    Cond[0].setImm(AVMCC::NE);
    return false;
  case AVMCC::NE:
    Cond[0].setImm(AVMCC::EQ);
    return false;
  case AVMCC::ULT:
    Cond[0].setImm(AVMCC::UGE);
    return false;
  case AVMCC::UGE:
    Cond[0].setImm(AVMCC::ULT);
    return false;
  case AVMCC::SLT:
    Cond[0].setImm(AVMCC::SGE);
    return false;
  case AVMCC::SGE:
    Cond[0].setImm(AVMCC::SLT);
    return false;
  default:
    return true;
  }
}

bool AVMInstrInfo::isPredicated(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  case AVM::CMOV16_PSEUDO:
  case AVM::CMOV32_PSEUDO:
  case AVM::CMOV_EQ:
  case AVM::CMOV_NE:
  case AVM::CMOV_ULT:
  case AVM::CMOV_UGE:
  case AVM::CMOV_SLT:
  case AVM::CMOV_SGE:
    return true;
  default:
    return false;
  }
}

bool AVMInstrInfo::isPredicable(const MachineInstr &MI) const {
  return MI.getOpcode() == AVM::COPY16_PSEUDO ||
         MI.getOpcode() == AVM::COPY32_PSEUDO;
}

bool AVMInstrInfo::PredicateInstruction(MachineInstr &MI,
                                        ArrayRef<MachineOperand> Pred) const {
  if (Pred.size() != 1 || !Pred[0].isImm() || !isPredicable(MI))
    return false;
  Register Dest = MI.getOperand(0).getReg();
  MI.setDesc(get(MI.getOpcode() == AVM::COPY32_PSEUDO ? AVM::CMOV32_PSEUDO
                                                      : AVM::CMOV16_PSEUDO));
  MI.addOperand(MachineOperand::CreateReg(Dest, false));
  MI.addOperand(MachineOperand::CreateImm(Pred[0].getImm()));
  MI.addOperand(MachineOperand::CreateReg(AVM::CC, false, true));
  return true;
}

bool AVMInstrInfo::ClobbersPredicate(MachineInstr &MI,
                                     std::vector<MachineOperand> &,
                                     bool) const {
  return MI.modifiesRegister(AVM::CC, &RI);
}

bool AVMInstrInfo::isProfitableToIfCvt(MachineBasicBlock &MBB, unsigned,
                                       unsigned, BranchProbability) const {
  if (MBB.getParent()->getFunction().hasOptSize())
    return false;
  unsigned Copies = countPredicableCopies(MBB);
  return Copies >= 1 && Copies <= 2;
}

bool AVMInstrInfo::isProfitableToIfCvt(MachineBasicBlock &TMBB, unsigned,
                                       unsigned, MachineBasicBlock &FMBB,
                                       unsigned, unsigned,
                                       BranchProbability) const {
  if (TMBB.getParent()->getFunction().hasOptSize())
    return false;
  unsigned TrueCopies = countPredicableCopies(TMBB);
  unsigned FalseCopies = countPredicableCopies(FMBB);
  return TrueCopies != std::numeric_limits<unsigned>::max() &&
         FalseCopies != std::numeric_limits<unsigned>::max() &&
         TrueCopies + FalseCopies >= 1 && TrueCopies + FalseCopies <= 2;
}

unsigned AVMInstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  auto IsUpperReg = [](Register Reg) {
    return Reg.isPhysical() && AVM::UpperGPR16RegClass.contains(Reg);
  };
  auto IsUpper = [&](const MachineOperand &MO) {
    return MO.isReg() && IsUpperReg(MO.getReg());
  };
  auto ImmediateSize = [&](Register Reg, uint16_t Value) {
    if (Value == 0 && IsUpperReg(Reg))
      return get(AVM::XOR).getSize();
    bool IsByte = Value <= 0xff;
    unsigned Opcode = IsUpperReg(Reg)
                          ? (IsByte ? AVM::LDI8 : AVM::LDI16)
                          : (IsByte ? AVM::COLDLDI8 : AVM::COLDLDI16);
    return get(Opcode).getSize();
  };
  auto MoveSize = [&](Register Dest, Register Src) {
    if (Dest == Src)
      return 0U;
    unsigned Opcode =
        IsUpperReg(Dest) && IsUpperReg(Src) ? AVM::MOV : AVM::MOV_RR;
    return get(Opcode).getSize();
  };
  auto CompactBinary = [&]() {
    return IsUpper(MI.getOperand(0)) && IsUpper(MI.getOperand(1)) &&
                   IsUpper(MI.getOperand(2))
               ? 1U
               : 2U;
  };

  switch (MI.getOpcode()) {
  case AVM::COPY16_PSEUDO:
    return MoveSize(MI.getOperand(0).getReg(), MI.getOperand(1).getReg());
  case AVM::COPY32_PSEUDO:
    return MI.getOperand(0).getReg() == MI.getOperand(1).getReg() ? 0 : 2;
  case AVM::LDI8_PSEUDO:
    return ImmediateSize(MI.getOperand(0).getReg(),
                         static_cast<uint8_t>(MI.getOperand(1).getImm()));
  case AVM::LDI16_PSEUDO:
    return ImmediateSize(MI.getOperand(0).getReg(),
                         static_cast<uint16_t>(MI.getOperand(1).getImm()));
  case AVM::DATA_ADDR_PSEUDO:
    return IsUpper(MI.getOperand(0)) ? get(AVM::LDI16).getSize()
                                     : get(AVM::COLDLDI16).getSize();
  case AVM::LDI32_PSEUDO: {
    const AVMRegisterInfo &TRI = getRegisterInfo();
    Register Dest = MI.getOperand(0).getReg();
    Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
    Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
    return ImmediateSize(Lo, static_cast<uint16_t>(MI.getOperand(1).getImm())) +
           ImmediateSize(Hi, static_cast<uint16_t>(MI.getOperand(2).getImm()));
  }
  case AVM::PROG_ADDR_PSEUDO:
    return get(AVM::LDI16).getSize() + get(AVM::LDI8).getSize();
  case AVM::PROG_CANON_PSEUDO:
    return get(AVM::ZEXT8).getSize();
  case AVM::ZEXT16_32_PSEUDO: {
    const AVMRegisterInfo &TRI = getRegisterInfo();
    Register Dest = MI.getOperand(0).getReg();
    Register Src = MI.getOperand(1).getReg();
    Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
    Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
    return MoveSize(Lo, Src) + ImmediateSize(Hi, 0);
  }
  case AVM::SEXT16_32_PSEUDO: {
    const AVMRegisterInfo &TRI = getRegisterInfo();
    Register Dest = MI.getOperand(0).getReg();
    Register Src = MI.getOperand(1).getReg();
    Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
    Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
    return MoveSize(Lo, Src) + MoveSize(Hi, Src) + get(AVM::ASR16I).getSize();
  }
  case AVM::SHL32_16_PSEUDO:
  case AVM::SRL32_16_PSEUDO:
  case AVM::SRA32_16_PSEUDO: {
    const AVMRegisterInfo &TRI = getRegisterInfo();
    Register Dest = MI.getOperand(0).getReg();
    Register Src = MI.getOperand(1).getReg();
    Register DestLo = TRI.getSubReg(Dest, AVM::sub_lo16);
    Register DestHi = TRI.getSubReg(Dest, AVM::sub_hi16);
    Register SrcLo = TRI.getSubReg(Src, AVM::sub_lo16);
    Register SrcHi = TRI.getSubReg(Src, AVM::sub_hi16);
    if (MI.getOpcode() == AVM::SHL32_16_PSEUDO)
      return MoveSize(DestHi, SrcLo) + ImmediateSize(DestLo, 0);
    if (MI.getOpcode() == AVM::SRL32_16_PSEUDO)
      return MoveSize(DestLo, SrcHi) + ImmediateSize(DestHi, 0);
    return MoveSize(DestLo, SrcHi) + MoveSize(DestHi, SrcHi) +
           get(AVM::ASR16I).getSize();
  }
  case AVM::ADD16_PSEUDO:
  case AVM::SUB16_PSEUDO:
  case AVM::AND16_PSEUDO:
  case AVM::OR16_PSEUDO:
  case AVM::XOR16_PSEUDO:
    return CompactBinary();
  case AVM::SEXT8_PSEUDO:
  case AVM::ZEXT8_PSEUDO:
  case AVM::INC16_PSEUDO:
  case AVM::DEC16_PSEUDO:
  case AVM::MUL8_PSEUDO:
  case AVM::MULU8W_PSEUDO:
  case AVM::MULS8W_PSEUDO:
  case AVM::MULSU8W_PSEUDO:
  case AVM::MUL16_PSEUDO:
  case AVM::UDIV16_PSEUDO:
  case AVM::UREM16_PSEUDO:
  case AVM::SDIV16_PSEUDO:
  case AVM::SREM16_PSEUDO:
  case AVM::TST8_PSEUDO:
  case AVM::TST16_PSEUDO:
  case AVM::CSET_PSEUDO:
  case AVM::CMOV16_PSEUDO:
  case AVM::LSR16_1_PSEUDO:
  case AVM::ASR16_1_PSEUDO:
  case AVM::LSL16I_PSEUDO:
  case AVM::LSR16I_PSEUDO:
  case AVM::ASR16I_PSEUDO:
  case AVM::SHL16V_PSEUDO:
  case AVM::LSR16V_PSEUDO:
  case AVM::ASR16V_PSEUDO:
    return 2;
  case AVM::ADDIS8_PSEUDO:
  case AVM::CMPIS8_PSEUDO:
    return IsUpper(MI.getOperand(0)) ? 2 : 3;
  case AVM::CMP16_PSEUDO:
    return IsUpper(MI.getOperand(0)) && IsUpper(MI.getOperand(1)) ? 1 : 2;
  case AVM::ADD32_PSEUDO:
  case AVM::SUB32_PSEUDO:
  case AVM::PROG_ADD_PSEUDO:
    return get(AVM::ADD32).getSize() + (MI.getOpcode() == AVM::PROG_ADD_PSEUDO
                                            ? get(AVM::ZEXT8).getSize()
                                            : 0);
  case AVM::AND32_PSEUDO:
  case AVM::OR32_PSEUDO:
  case AVM::XOR32_PSEUDO:
    return 2 * get(AVM::XOR_RR).getSize();
  case AVM::BSWAP32_PSEUDO:
    return 2 * get(AVM::BSWAP16).getSize() + 3 * get(AVM::XOR_RR).getSize();
  case AVM::CMP32_PSEUDO:
    return get(AVM::CMP32).getSize();
  case AVM::LOAD24_PSEUDO:
    return get(AVM::GPLD16_POST).getSize() + get(AVM::DPLD8U).getSize();
  case AVM::STORE24_PSEUDO:
    return get(AVM::GPST16_POST).getSize() + get(AVM::DPST8).getSize();
  case AVM::PLOAD8U_PSEUDO:
  case AVM::PLOAD8S_PSEUDO:
    return get(AVM::LDP8U).getSize();
  case AVM::PLOAD16_PSEUDO:
    return get(AVM::LDP16).getSize();
  case AVM::PLOAD24_PSEUDO:
    return get(AVM::LDP24).getSize();
  case AVM::PLOAD32_PSEUDO:
    return get(AVM::LDP32).getSize();
  case AVM::PLOAD8U_POST_PSEUDO:
    return get(AVM::LDP8U_POST).getSize();
  case AVM::PLOAD16_POST_PSEUDO:
    return get(AVM::LDP16_POST).getSize();
  case AVM::PLOAD24_POST_PSEUDO:
    return get(AVM::LDP24_POST).getSize();
  case AVM::PLOAD32_POST_PSEUDO:
    return get(AVM::LDP32_POST).getSize();
  case AVM::FADD_PSEUDO:
  case AVM::FSUB_PSEUDO:
  case AVM::FMUL_PSEUDO:
  case AVM::FDIV_PSEUDO:
  case AVM::FMIN_PSEUDO:
  case AVM::FMAX_PSEUDO:
  case AVM::FNEG_PSEUDO:
  case AVM::FABS_PSEUDO:
  case AVM::FSQRT_PSEUDO:
  case AVM::S16TOF_PSEUDO:
  case AVM::U16TOF_PSEUDO:
  case AVM::S32TOF_PSEUDO:
  case AVM::U32TOF_PSEUDO:
  case AVM::FTOS16_PSEUDO:
  case AVM::FTOU16_PSEUDO:
  case AVM::FTOS32_PSEUDO:
  case AVM::FTOU32_PSEUDO:
  case AVM::FCMP_PSEUDO:
  case AVM::FCLASS_PSEUDO:
    return 3;
  case AVM::CMOV32_PSEUDO:
    return 4;
  case AVM::SHL16_SMALL_PSEUDO: {
    unsigned Count = MI.getOperand(2).getImm();
    return Count * (IsUpper(MI.getOperand(0)) ? 1U : 2U);
  }
  case AVM::BR_CC_PSEUDO:
    return 6;
  case AVM::JMP_PSEUDO:
    return 4;
  default:
    return MI.getDesc().getSize();
  }
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
  auto IsUpper = [](Register Reg) {
    return Reg.isPhysical() && AVM::UpperGPR16RegClass.contains(Reg);
  };
  auto ImmediateLatency = [&](Register Reg, uint16_t Value) {
    if (Value == 0 && IsUpper(Reg))
      return Fixed(AVMCostKind::ClrUpper);
    if (Value <= 0xff)
      return Fixed(IsUpper(Reg) ? AVMCostKind::Ldi8Upper
                                : AVMCostKind::Ldi8Lower);
    return Fixed(IsUpper(Reg) ? AVMCostKind::Ldi16Upper
                              : AVMCostKind::Ldi16Lower);
  };
  auto MoveLatency = [&](Register Dest, Register Src) {
    if (Dest == Src)
      return 0U;
    return Fixed(IsUpper(Dest) && IsUpper(Src) ? AVMCostKind::MovUpper
                                               : AVMCostKind::MovFull);
  };

  switch (MI.getOpcode()) {
  case AVM::COPY16_PSEUDO:
    return MoveLatency(MI.getOperand(0).getReg(), MI.getOperand(1).getReg());
  case AVM::COPY32_PSEUDO: {
    Register Dest = MI.getOperand(0).getReg();
    Register Src = MI.getOperand(1).getReg();
    if (Dest == Src)
      return 0;
    if (AVM::UpperGPR32RegClass.contains(Dest, Src))
      return 2 * Fixed(AVMCostKind::MovUpper);
    return Fixed(AVMCostKind::Mov32Full);
  }
  case AVM::LDI8_PSEUDO:
    return ImmediateLatency(MI.getOperand(0).getReg(),
                            static_cast<uint8_t>(MI.getOperand(1).getImm()));
  case AVM::LDI16_PSEUDO:
    return ImmediateLatency(MI.getOperand(0).getReg(),
                            static_cast<uint16_t>(MI.getOperand(1).getImm()));
  case AVM::LDI32_PSEUDO: {
    const AVMRegisterInfo &TRI = getRegisterInfo();
    Register Dest = MI.getOperand(0).getReg();
    Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
    Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
    return ImmediateLatency(Lo,
                            static_cast<uint16_t>(MI.getOperand(1).getImm())) +
           ImmediateLatency(Hi,
                            static_cast<uint16_t>(MI.getOperand(2).getImm()));
  }
  case AVM::PROG_ADDR_PSEUDO:
    return Fixed(AVMCostKind::Ldi16Upper) + Fixed(AVMCostKind::Ldi8Upper);
  case AVM::PROG_CANON_PSEUDO:
    return Fixed(AVMCostKind::ZExt8);
  case AVM::ZEXT16_32_PSEUDO: {
    const AVMRegisterInfo &TRI = getRegisterInfo();
    Register Dest = MI.getOperand(0).getReg();
    Register Src = MI.getOperand(1).getReg();
    Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
    Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
    return MoveLatency(Lo, Src) + ImmediateLatency(Hi, 0);
  }
  case AVM::SEXT16_32_PSEUDO: {
    const AVMRegisterInfo &TRI = getRegisterInfo();
    Register Dest = MI.getOperand(0).getReg();
    Register Src = MI.getOperand(1).getReg();
    Register Lo = TRI.getSubReg(Dest, AVM::sub_lo16);
    Register Hi = TRI.getSubReg(Dest, AVM::sub_hi16);
    return MoveLatency(Lo, Src) + MoveLatency(Hi, Src) +
           AVM::getShiftCycles(AVMCostKind::Asr16I, 15);
  }
  case AVM::SHL32_16_PSEUDO:
  case AVM::SRL32_16_PSEUDO:
  case AVM::SRA32_16_PSEUDO: {
    const AVMRegisterInfo &TRI = getRegisterInfo();
    Register Dest = MI.getOperand(0).getReg();
    Register Src = MI.getOperand(1).getReg();
    Register DestLo = TRI.getSubReg(Dest, AVM::sub_lo16);
    Register DestHi = TRI.getSubReg(Dest, AVM::sub_hi16);
    Register SrcLo = TRI.getSubReg(Src, AVM::sub_lo16);
    Register SrcHi = TRI.getSubReg(Src, AVM::sub_hi16);
    if (MI.getOpcode() == AVM::SHL32_16_PSEUDO)
      return MoveLatency(DestHi, SrcLo) + ImmediateLatency(DestLo, 0);
    if (MI.getOpcode() == AVM::SRL32_16_PSEUDO)
      return MoveLatency(DestLo, SrcHi) + ImmediateLatency(DestHi, 0);
    return MoveLatency(DestLo, SrcHi) + MoveLatency(DestHi, SrcHi) +
           AVM::getShiftCycles(AVMCostKind::Asr16I, 15);
  }
  case AVM::ADD32_PSEUDO:
    return Fixed(AVMCostKind::Add32);
  case AVM::SUB32_PSEUDO:
    return Fixed(AVMCostKind::Sub32);
  case AVM::AND32_PSEUDO:
    return 2 * Fixed(AVMCostKind::AndUpper);
  case AVM::OR32_PSEUDO:
    return 2 * Fixed(AVMCostKind::OrUpper);
  case AVM::XOR32_PSEUDO:
    return 2 * Fixed(AVMCostKind::XorUpper);
  case AVM::PROG_ADD_PSEUDO:
    return Fixed(AVMCostKind::Add32) + Fixed(AVMCostKind::ZExt8);
  case AVM::BSWAP32_PSEUDO:
    return 2 * Fixed(AVMCostKind::BSwap16) + 3 * Fixed(AVMCostKind::XorUpper);
  case AVM::CMP32_PSEUDO:
    return Fixed(AVMCostKind::Cmp32);
  case AVM::LOAD24_PSEUDO:
    return Fixed(AVMCostKind::Ld16PostIncGeneral) +
           Fixed(AVMCostKind::Ld8UDisplaced);
  case AVM::STORE24_PSEUDO:
    return Fixed(AVMCostKind::St16PostIncGeneral) +
           Fixed(AVMCostKind::St8Displaced);
  case AVM::PLOAD8U_PSEUDO:
    return Fixed(AVMCostKind::Ldp8U);
  case AVM::PLOAD8S_PSEUDO:
    return Fixed(AVMCostKind::Ldp8S);
  case AVM::PLOAD16_PSEUDO:
    return Fixed(AVMCostKind::Ldp16);
  case AVM::PLOAD24_PSEUDO:
    return Fixed(AVMCostKind::Ldp24);
  case AVM::PLOAD32_PSEUDO:
    return Fixed(AVMCostKind::Ldp32);
  case AVM::PLOAD8U_POST_PSEUDO:
    return Fixed(AVMCostKind::Ldp8UPostInc);
  case AVM::PLOAD16_POST_PSEUDO:
    return Fixed(AVMCostKind::Ldp16PostInc);
  case AVM::PLOAD24_POST_PSEUDO:
    return Fixed(AVMCostKind::Ldp24PostInc);
  case AVM::PLOAD32_POST_PSEUDO:
    return Fixed(AVMCostKind::Ldp32PostInc);
  case AVM::FADD_PSEUDO:
    return Typical(AVMCostKind::FAdd);
  case AVM::FSUB_PSEUDO:
    return Typical(AVMCostKind::FSub);
  case AVM::FMUL_PSEUDO:
    return Typical(AVMCostKind::FMul);
  case AVM::FDIV_PSEUDO:
    return Typical(AVMCostKind::FDiv);
  case AVM::FMIN_PSEUDO:
    return Typical(AVMCostKind::FMin);
  case AVM::FMAX_PSEUDO:
    return Typical(AVMCostKind::FMax);
  case AVM::FSQRT_PSEUDO:
    return Typical(AVMCostKind::FSqrt);
  case AVM::FNEG_PSEUDO:
    return Fixed(AVMCostKind::FNeg);
  case AVM::FABS_PSEUDO:
    return Fixed(AVMCostKind::FAbs);
  case AVM::S16TOF_PSEUDO:
    return Typical(AVMCostKind::S16ToF);
  case AVM::U16TOF_PSEUDO:
    return Typical(AVMCostKind::U16ToF);
  case AVM::FTOS16_PSEUDO:
    return Typical(AVMCostKind::FToS16);
  case AVM::FTOU16_PSEUDO:
    return Typical(AVMCostKind::FToU16);
  case AVM::S32TOF_PSEUDO:
    return Typical(AVMCostKind::S32ToF);
  case AVM::U32TOF_PSEUDO:
    return Typical(AVMCostKind::U32ToF);
  case AVM::FTOS32_PSEUDO:
    return Typical(AVMCostKind::FToS32);
  case AVM::FTOU32_PSEUDO:
    return Typical(AVMCostKind::FToU32);
  case AVM::FCMP_PSEUDO:
    return Typical(AVMCostKind::FCmp);
  case AVM::FCLASS_PSEUDO:
    return Typical(AVMCostKind::FClass);
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
  case AVM::LSR16_1_PSEUDO:
    return Fixed(AVMCostKind::Lsr16One);
  case AVM::ASR16_1_PSEUDO:
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
  case AVM::DPLD8U:
    return Fixed(AVMCostKind::Ld8UDisplaced);
  case AVM::DPLD16:
    return Fixed(AVMCostKind::Ld16Displaced);
  case AVM::DPST8:
    return Fixed(AVMCostKind::St8Displaced);
  case AVM::DPST16:
    return Fixed(AVMCostKind::St16Displaced);
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
  case AVM::UDIV16_PSEUDO:
    return Typical(AVMCostKind::UDiv16);
  case AVM::UREM16_PSEUDO:
    return Typical(AVMCostKind::URem16);
  case AVM::SDIV16_PSEUDO:
    return Typical(AVMCostKind::SDiv16);
  case AVM::SREM16_PSEUDO:
    return Typical(AVMCostKind::SRem16);
  case AVM::SHL16V:
  case AVM::LSR16V:
  case AVM::ASR16V:
  case AVM::SHL16V_PSEUDO:
  case AVM::LSR16V_PSEUDO:
  case AVM::ASR16V_PSEUDO:
    return 60;
  case AVM::LSL16I:
  case AVM::LSL16I_PSEUDO:
    return AVM::getShiftCycles(AVMCostKind::Lsl16I, MI.getOperand(2).getImm());
  case AVM::LSR16I:
  case AVM::LSR16I_PSEUDO:
    return AVM::getShiftCycles(AVMCostKind::Lsr16I, MI.getOperand(2).getImm());
  case AVM::ASR16I:
  case AVM::ASR16I_PSEUDO:
    return AVM::getShiftCycles(AVMCostKind::Asr16I, MI.getOperand(2).getImm());
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
