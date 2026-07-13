#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "AVMGenInstrInfo.inc"

AVMInstrInfo::AVMInstrInfo(const AVMSubtarget &STI)
    : AVMGenInstrInfo(STI, RI), RI() {}

void AVMInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MI,
                               const DebugLoc &DL, Register DestReg,
                               Register SrcReg, bool KillSrc, bool,
                               bool) const {
  unsigned Opc =
      AVM::CGPR16RegClass.contains(DestReg, SrcReg) ? AVM::MOVC : AVM::MOV16_E3;
  BuildMI(MBB, MI, DL, get(Opc), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
}

void AVMInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MI,
                                       Register SrcReg, bool IsKill, int FI,
                                       const TargetRegisterClass *RC, Register,
                                       MachineInstr::MIFlag Flags) const {
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  auto *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FI), MachineMemOperand::MOStore,
      MFI.getObjectSize(FI), MFI.getObjectAlign(FI));
  unsigned Opc =
      RC->hasSubClassEq(&AVM::GPR8RegClass) ? AVM::STSP8 : AVM::STSP16;
  BuildMI(MBB, MI, DebugLoc(), get(Opc))
      .addFrameIndex(FI)
      .addReg(SrcReg, getKillRegState(IsKill))
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}

void AVMInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        Register DestReg, int FI,
                                        const TargetRegisterClass *RC, Register,
                                        unsigned,
                                        MachineInstr::MIFlag Flags) const {
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  auto *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FI), MachineMemOperand::MOLoad,
      MFI.getObjectSize(FI), MFI.getObjectAlign(FI));
  unsigned Opc =
      RC->hasSubClassEq(&AVM::GPR8RegClass) ? AVM::LDSP8 : AVM::LDSP16;
  BuildMI(MBB, MI, DebugLoc(), get(Opc), DestReg)
      .addFrameIndex(FI)
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}
