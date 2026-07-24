//===-- AVMRegisterInfo.cpp - AVM register information -------------------===//

#include "AVMRegisterInfo.h"
#include "AVM.h"
#include "AVMCostModel.h"
#include "AVMFrameLowering.h"
#include "AVMInstrInfo.h"
#include "AVMSubtarget.h"
#include "AVMSystemServiceInfo.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"

#include <algorithm>

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "AVMGenRegisterInfo.inc"

AVMRegisterInfo::AVMRegisterInfo() : AVMGenRegisterInfo(0) {}

const uint16_t *
AVMRegisterInfo::getCalleeSavedRegs(const MachineFunction *) const {
  return CSR_AVM_SaveList;
}

const uint32_t *AVMRegisterInfo::getCallPreservedMask(const MachineFunction &,
                                                      CallingConv::ID) const {
  return CSR_AVM_CALL_RegMask;
}

BitVector AVMRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  markSuperRegs(Reserved, AVM::SP);
  markSuperRegs(Reserved, AVM::PC);
  markSuperRegs(Reserved, AVM::CC);
  if (MF.getSubtarget<AVMSubtarget>().getFrameLowering()->hasFP(MF))
    markSuperRegs(Reserved, AVM::R3);
  return Reserved;
}

static bool crossesFixedServiceClassBoundary(const TargetRegisterClass *SrcRC,
                                             const TargetRegisterClass *DstRC,
                                             const TargetRegisterClass *NewRC) {
  const bool SrcFixed = isAVMFixedServiceRegisterClass(SrcRC);
  const bool DstFixed = isAVMFixedServiceRegisterClass(DstRC);
  const bool NewFixed = isAVMFixedServiceRegisterClass(NewRC);

  if (SrcFixed || DstFixed)
    return SrcRC != DstRC;

  return NewFixed;
}

// Fixed service register classes describe short ABI setup/result intervals.
// They must not absorb a general live interval through coalescing. General and
// fixed intervals must remain separate so the general interval remains
// allocatable and spillable.
bool AVMRegisterInfo::shouldCoalesce(MachineInstr *MI,
                                     const TargetRegisterClass *SrcRC, unsigned,
                                     const TargetRegisterClass *DstRC, unsigned,
                                     const TargetRegisterClass *NewRC,
                                     LiveIntervals &) const {
  if (MI && MI->getFlag(MachineInstr::NoMerge))
    return false;

  if (crossesFixedServiceClassBoundary(SrcRC, DstRC, NewRC))
    return false;

  return true;
}

namespace {
void emitAddress(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                 const DebugLoc &DL, const AVMInstrInfo &TII, Register Dest,
                 Register Base, uint64_t Offset) {
  if (Base == AVM::SP)
    BuildMI(MBB, MI, DL, TII.get(AVM::GETSP), Dest);
  else
    BuildMI(MBB, MI, DL, TII.get(AVM::COPY16_PSEUDO), Dest).addReg(Base);

  while (Offset) {
    int64_t Chunk = std::min<uint64_t>(Offset, 127);
    unsigned Opcode =
        AVM::UpperGPR16RegClass.contains(Dest) ? AVM::ADDIS8 : AVM::COLDADDIS8;
    BuildMI(MBB, MI, DL, TII.get(Opcode), Dest).addReg(Dest).addImm(Chunk);
    Offset -= Chunk;
  }
}

unsigned preferCompact(const MachineFunction &MF, const AVMInstrInfo &TII,
                       bool CompactLegal, unsigned CompactOpcode,
                       AVM::AVMCostKind CompactCost, unsigned FullOpcode,
                       AVM::AVMCostKind FullCost) {
  if (!CompactLegal)
    return FullOpcode;
  if (MF.getFunction().hasOptSize())
    return TII.get(CompactOpcode).getSize() <= TII.get(FullOpcode).getSize()
               ? CompactOpcode
               : FullOpcode;

  // Both choices replace the same instruction in the same block, so its block
  // frequency is a common factor.  Compare their measured interpreter cycles.
  return AVM::getFixedCycles(CompactCost) <= AVM::getFixedCycles(FullCost)
             ? CompactOpcode
             : FullOpcode;
}

unsigned getStackLoadOpcode(const MachineFunction &MF, const AVMInstrInfo &TII,
                            Register Reg, uint64_t Offset) {
  return preferCompact(
      MF, TII, AVM::UpperGPR16RegClass.contains(Reg) && isUInt<4>(Offset),
      AVM::LDSP16_COMPACT, AVM::AVMCostKind::LdSp16Short, AVM::LDSP16,
      AVM::AVMCostKind::LdSp16Cold);
}

unsigned getStackStoreOpcode(const MachineFunction &MF, const AVMInstrInfo &TII,
                             Register Reg, uint64_t Offset) {
  return preferCompact(
      MF, TII, AVM::UpperGPR16RegClass.contains(Reg) && isUInt<4>(Offset),
      AVM::STSP16_COMPACT, AVM::AVMCostKind::StSp16Short, AVM::STSP16,
      AVM::AVMCostKind::StSp16Cold);
}
} // namespace

bool AVMRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                          int SPAdj, unsigned FIOperandNum,
                                          RegScavenger *RS) const {
  MachineInstr &Old = *MI;
  MachineBasicBlock &MBB = *Old.getParent();
  MachineFunction &MF = *MBB.getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const AVMSubtarget &STI = MF.getSubtarget<AVMSubtarget>();
  const AVMInstrInfo &TII = *STI.getInstrInfo();
  const AVMFrameLowering &TFI = *STI.getFrameLowering();
  int FrameIndex = Old.getOperand(FIOperandNum).getIndex();
  bool IsScavengingSlot = RS && RS->isScavengingFrameIndex(FrameIndex);
  Register Base = TFI.hasFP(MF) && !IsScavengingSlot ? AVM::R3 : AVM::SP;
  int64_t Offset = MFI.getObjectOffset(FrameIndex) + MFI.getStackSize();
  Offset += Old.getOperand(FIOperandNum + 1).getImm();
  if (Base == AVM::SP)
    Offset += SPAdj;
  if (!isUInt<16>(Offset))
    report_fatal_error("AVM fixed-frame offset exceeds the data address space");

  DebugLoc DL = Old.getDebugLoc();
  unsigned Opcode = Old.getOpcode();
  auto CloneMemRefs = [&](MachineInstrBuilder MIB) -> MachineInstrBuilder {
    return MIB.cloneMemRefs(Old);
  };
  auto BuildDisplacedLoad = [&](unsigned LoadOpcode, Register Dest,
                                Register Address, int64_t Displacement,
                                unsigned AddressState = 0) {
    return CloneMemRefs(BuildMI(MBB, MI, DL, TII.get(LoadOpcode), Dest)
                            .addReg(Address, AddressState)
                            .addImm(Displacement));
  };
  auto BuildDisplacedStore =
      [&](unsigned StoreOpcode, Register Address, int64_t Displacement,
          Register Src, unsigned AddressState = 0, unsigned SrcState = 0) {
        return CloneMemRefs(BuildMI(MBB, MI, DL, TII.get(StoreOpcode))
                                .addReg(Address, AddressState)
                                .addImm(Displacement)
                                .addReg(Src, SrcState));
      };
  auto GetScratch = [&]() {
    assert(RS && "AVM far frame stores require register scavenging");
    Register Scratch =
        RS->scavengeRegisterBackwards(AVM::GPR16RegClass, MI, false, SPAdj);
    assert(Scratch && "AVM register scavenging failed");
    RS->setRegUsed(Scratch);
    return Scratch;
  };

  if (Opcode == AVM::FRAMEADDR_PSEUDO) {
    Register Dest = Old.getOperand(0).getReg();
    if (Base == AVM::SP && isUInt<8>(Offset))
      BuildMI(MBB, MI, DL, TII.get(AVM::LEASP), Dest).addImm(Offset);
    else
      emitAddress(MBB, MI, DL, TII, Dest, Base, Offset);
    Old.eraseFromParent();
    return true;
  }

  bool IsLoad = Opcode == AVM::STACK_LOAD8U_PSEUDO ||
                Opcode == AVM::STACK_LOAD8S_PSEUDO ||
                Opcode == AVM::STACK_LOAD16_PSEUDO ||
                Opcode == AVM::STACK_LOAD24_PSEUDO ||
                Opcode == AVM::STACK_LOAD32_PSEUDO;
  bool IsPair = Opcode == AVM::STACK_LOAD24_PSEUDO ||
                Opcode == AVM::STACK_LOAD32_PSEUDO ||
                Opcode == AVM::STACK_STORE24_PSEUDO ||
                Opcode == AVM::STACK_STORE32_PSEUDO;
  bool Is24 =
      Opcode == AVM::STACK_LOAD24_PSEUDO || Opcode == AVM::STACK_STORE24_PSEUDO;
  Register ValueReg = Old.getOperand(IsLoad ? 0 : 2).getReg();
  bool IsKill = !IsLoad && Old.getOperand(2).isKill();

  if (Base == AVM::SP && isUInt<8>(Offset) &&
      (!IsPair || isUInt<8>(Offset + 2))) {
    if (!IsPair) {
      unsigned NewOpcode;
      bool SignExtendAfterLoad = false;
      if (Opcode == AVM::STACK_LOAD8U_PSEUDO)
        NewOpcode = preferCompact(
            MF, TII,
            AVM::UpperGPR16RegClass.contains(ValueReg) && isUInt<4>(Offset),
            AVM::LDSP8U_COMPACT, AVM::AVMCostKind::LdSp8UShort, AVM::LDSP8U,
            AVM::AVMCostKind::LdSp8UCold);
      else if (Opcode == AVM::STACK_LOAD8S_PSEUDO) {
        SignExtendAfterLoad =
            AVM::UpperGPR16RegClass.contains(ValueReg) && isUInt<4>(Offset);
        NewOpcode = SignExtendAfterLoad ? AVM::LDSP8U_COMPACT : AVM::LDSP8S;
      } else if (IsLoad)
        NewOpcode = getStackLoadOpcode(MF, TII, ValueReg, Offset);
      else if (Opcode == AVM::STACK_STORE8_PSEUDO)
        NewOpcode = preferCompact(
            MF, TII,
            AVM::UpperGPR16RegClass.contains(ValueReg) && isUInt<4>(Offset),
            AVM::STSP8_COMPACT, AVM::AVMCostKind::StSp8Short, AVM::STSP8,
            AVM::AVMCostKind::StSp8Cold);
      else
        NewOpcode = getStackStoreOpcode(MF, TII, ValueReg, Offset);

      MachineInstrBuilder MIB =
          IsLoad ? BuildMI(MBB, MI, DL, TII.get(NewOpcode), ValueReg)
                 : BuildMI(MBB, MI, DL, TII.get(NewOpcode));
      MIB.addImm(Offset);
      if (!IsLoad)
        MIB.addReg(ValueReg, getKillRegState(IsKill));
      CloneMemRefs(MIB);
      if (SignExtendAfterLoad)
        BuildMI(MBB, MI, DL, TII.get(AVM::SEXT8), ValueReg).addReg(ValueReg);
      Old.eraseFromParent();
      return true;
    }

    Register Lo = getSubReg(ValueReg, AVM::sub_lo16);
    Register Hi = getSubReg(ValueReg, AVM::sub_hi16);
    if (IsLoad) {
      CloneMemRefs(BuildMI(MBB, MI, DL,
                           TII.get(getStackLoadOpcode(MF, TII, Lo, Offset)), Lo)
                       .addImm(Offset));
      unsigned HiOpcode =
          Is24 ? preferCompact(MF, TII,
                               AVM::UpperGPR16RegClass.contains(Hi) &&
                                   isUInt<4>(Offset + 2),
                               AVM::LDSP8U_COMPACT,
                               AVM::AVMCostKind::LdSp8UShort, AVM::LDSP8U,
                               AVM::AVMCostKind::LdSp8UCold)
               : getStackLoadOpcode(MF, TII, Hi, Offset + 2);
      CloneMemRefs(
          BuildMI(MBB, MI, DL, TII.get(HiOpcode), Hi).addImm(Offset + 2));
    } else {
      CloneMemRefs(BuildMI(MBB, MI, DL,
                           TII.get(getStackStoreOpcode(MF, TII, Lo, Offset)))
                       .addImm(Offset)
                       .addReg(Lo, getKillRegState(IsKill)));
      unsigned HiOpcode =
          Is24 ? preferCompact(MF, TII,
                               AVM::UpperGPR16RegClass.contains(Hi) &&
                                   isUInt<4>(Offset + 2),
                               AVM::STSP8_COMPACT, AVM::AVMCostKind::StSp8Short,
                               AVM::STSP8, AVM::AVMCostKind::StSp8Cold)
               : getStackStoreOpcode(MF, TII, Hi, Offset + 2);
      CloneMemRefs(BuildMI(MBB, MI, DL, TII.get(HiOpcode))
                       .addImm(Offset + 2)
                       .addReg(Hi, getKillRegState(IsKill)));
    }
    Old.eraseFromParent();
    return true;
  }

  Register Address;
  if (IsLoad)
    Address = IsPair ? Register(getSubReg(ValueReg, AVM::sub_lo16)) : ValueReg;
  else
    Address = GetScratch();
  emitAddress(MBB, MI, DL, TII, Address, Base, Offset);

  if (IsPair) {
    if (Is24) {
      Register Lo = getSubReg(ValueReg, AVM::sub_lo16);
      Register Hi = getSubReg(ValueReg, AVM::sub_hi16);
      if (IsLoad) {
        BuildDisplacedLoad(AVM::DPLD8U, Hi, Address, 2);
        BuildDisplacedLoad(AVM::DPLD16, Lo, Address, 0, RegState::Kill);
      } else {
        BuildDisplacedStore(AVM::DPST16, Address, 0, Lo, 0,
                            getKillRegState(IsKill));
        BuildDisplacedStore(AVM::DPST8, Address, 2, Hi, RegState::Kill,
                            getKillRegState(IsKill));
      }
    } else if (IsLoad)
      CloneMemRefs(BuildMI(MBB, MI, DL, TII.get(AVM::LD32), ValueReg)
                       .addReg(Address, RegState::Kill));
    else
      CloneMemRefs(BuildMI(MBB, MI, DL, TII.get(AVM::ST32))
                       .addReg(Address, RegState::Kill)
                       .addReg(ValueReg, getKillRegState(IsKill)));
  } else if (IsLoad) {
    unsigned LoadOpcode =
        Opcode == AVM::STACK_LOAD16_PSEUDO ? AVM::DPLD16 : AVM::DPLD8U;
    BuildDisplacedLoad(LoadOpcode, ValueReg, Address, 0, RegState::Kill);
    if (Opcode == AVM::STACK_LOAD8S_PSEUDO)
      BuildMI(MBB, MI, DL, TII.get(AVM::SEXT8), ValueReg).addReg(ValueReg);
  } else {
    unsigned StoreOpcode =
        Opcode == AVM::STACK_STORE8_PSEUDO ? AVM::DPST8 : AVM::DPST16;
    BuildDisplacedStore(StoreOpcode, Address, 0, ValueReg, RegState::Kill,
                        getKillRegState(IsKill));
  }

  Old.eraseFromParent();
  return true;
}

Register AVMRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return MF.getSubtarget<AVMSubtarget>().getFrameLowering()->hasFP(MF)
             ? AVM::R3
             : AVM::SP;
}

const TargetRegisterClass *
AVMRegisterInfo::getPointerRegClass(unsigned Kind) const {
  assert(Kind == 0 && "AVM has no alternate pointer register class yet");
  return &AVM::PTR16RegClass;
}
