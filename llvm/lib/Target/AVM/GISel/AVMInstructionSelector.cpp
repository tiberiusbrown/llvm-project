#include "AVMInstructionSelector.h"
#include "../AVMInstrInfo.h"
#include "../AVMSubtarget.h"
#include "../MCTargetDesc/AVMMCTargetDesc.h"
#include "AVMRegisterBankInfo.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/IR/InstrTypes.h"

using namespace llvm;

namespace {
class AVMInstructionSelector final : public InstructionSelector {
  const AVMInstrInfo &TII;
  const AVMRegisterInfo &TRI;
  const AVMRegisterBankInfo &RBI;

  bool constrain(MachineInstr &I) const {
    return constrainSelectedInstRegOperands(I, TII, TRI, RBI);
  }

public:
  AVMInstructionSelector(const AVMSubtarget &STI,
                         const AVMRegisterBankInfo &RBI)
      : TII(*STI.getInstrInfo()), TRI(*STI.getRegisterInfo()), RBI(RBI) {}

  bool select(MachineInstr &I) override;
  void setupGeneratedPerFunctionState(MachineFunction &) override {}
};
} // namespace

static std::optional<unsigned> mapPredicate(CmpInst::Predicate &Pred,
                                            bool &Swap) {
  Swap = false;
  switch (Pred) {
  case CmpInst::ICMP_EQ:
    return 0;
  case CmpInst::ICMP_NE:
    return 1;
  case CmpInst::ICMP_ULT:
    return 2;
  case CmpInst::ICMP_UGE:
    return 3;
  case CmpInst::ICMP_SLT:
    return 4;
  case CmpInst::ICMP_SGE:
    return 5;
  case CmpInst::ICMP_ULE:
    return 6;
  case CmpInst::ICMP_UGT:
    return 7;
  case CmpInst::ICMP_SLE:
    Swap = true;
    return 5;
  case CmpInst::ICMP_SGT:
    Swap = true;
    return 4;
  default:
    return std::nullopt;
  }
}

bool AVMInstructionSelector::select(MachineInstr &I) {
  MachineRegisterInfo &MRI = I.getMF()->getRegInfo();
  MachineIRBuilder MIB(I);
  using namespace TargetOpcode;

  if (I.getOpcode() == COPY) {
    for (MachineOperand &MO : I.operands()) {
      if (!MO.isReg() || !MO.getReg().isVirtual())
        continue;
      LLT Ty = MRI.getType(MO.getReg());
      const TargetRegisterClass &RC =
          Ty.getSizeInBits() <= 8 ? AVM::CGPR8RegClass : AVM::CGPR16RegClass;
      if (!RBI.constrainGenericRegister(MO.getReg(), RC, MRI))
        return false;
    }
    return true;
  }

  if (!isPreISelGenericOpcode(I.getOpcode())) {
    if (I.getOpcode() == AVM::GETSP_G) {
      I.setDesc(TII.get(AVM::GETSP));
      return constrain(I);
    }
    return true;
  }

  switch (I.getOpcode()) {
  case G_CONSTANT: {
    int64_t Value = I.getOperand(1).getCImm()->getValue().getSExtValue();
    I.getOperand(1).ChangeToImmediate(Value);
    I.setDesc(TII.get(AVM::LDI16));
    return constrain(I);
  }
  case G_GLOBAL_VALUE:
    I.setDesc(TII.get(AVM::LDI16));
    return constrain(I);
  case G_ADD:
  case G_PTR_ADD:
    I.setDesc(TII.get(AVM::ADDC));
    return constrain(I);
  case G_SUB:
    I.setDesc(TII.get(AVM::SUBC));
    return constrain(I);
  case G_AND:
    I.setDesc(TII.get(AVM::AND16));
    return constrain(I);
  case G_OR:
    I.setDesc(TII.get(AVM::OR16));
    return constrain(I);
  case G_XOR:
    I.setDesc(TII.get(AVM::XOR16));
    return constrain(I);
  case G_FRAME_INDEX: {
    Register Dst = I.getOperand(0).getReg();
    Register Base = MRI.createVirtualRegister(&AVM::CGPR16RegClass);
    MachineInstr *GetSP = MIB.buildInstr(AVM::GETSP).addDef(Base);
    MachineInstr *Add = MIB.buildInstr(AVM::ADDI16)
                            .addDef(Dst)
                            .addUse(Base)
                            .add(I.getOperand(1));
    if (!constrain(*GetSP) || !constrain(*Add))
      return false;
    I.eraseFromParent();
    return true;
  }
  case G_ICMP: {
    Register Dst = I.getOperand(0).getReg();
    CmpInst::Predicate Pred =
        static_cast<CmpInst::Predicate>(I.getOperand(1).getPredicate());
    Register LHS = I.getOperand(2).getReg();
    Register RHS = I.getOperand(3).getReg();
    bool Swap;
    std::optional<unsigned> CC = mapPredicate(Pred, Swap);
    if (!CC)
      return false;
    if (Swap)
      std::swap(LHS, RHS);
    MachineInstr *Cmp = MIB.buildInstr(AVM::CMP16C).addUse(LHS).addUse(RHS);
    MachineInstr *Set = MIB.buildInstr(AVM::CSET).addDef(Dst).addImm(*CC);
    if (!constrain(*Cmp) || !constrain(*Set))
      return false;
    I.eraseFromParent();
    return true;
  }
  case G_BRCOND: {
    Register Cond = I.getOperand(0).getReg();
    MachineBasicBlock *Target = I.getOperand(1).getMBB();
    MachineInstr *Test = MIB.buildInstr(AVM::TST16C).addUse(Cond);
    MachineInstr *Branch = MIB.buildInstr(AVM::BRNE).addMBB(Target);
    if (!constrain(*Test) || !constrain(*Branch))
      return false;
    I.eraseFromParent();
    return true;
  }
  case G_BR:
    I.setDesc(TII.get(AVM::JMP_REL8));
    return constrain(I);
  case G_LOAD: {
    const MachineMemOperand &MMO = **I.memoperands_begin();
    I.setDesc(TII.get(MMO.getMemoryType().getSizeInBits() <= 8 ? AVM::LD8C
                                                               : AVM::LD16C));
    return constrain(I);
  }
  case G_STORE: {
    Register Value = I.getOperand(0).getReg();
    Register Address = I.getOperand(1).getReg();
    MachineMemOperand *MMO = *I.memoperands_begin();
    MachineInstr *Store =
        MIB.buildInstr(MMO->getMemoryType().getSizeInBits() <= 8 ? AVM::ST8C
                                                                 : AVM::ST16C)
            .addUse(Address)
            .addUse(Value)
            .addMemOperand(MMO);
    if (!constrain(*Store))
      return false;
    I.eraseFromParent();
    return true;
  }
  case G_ZEXT:
  case G_ANYEXT:
    I.setDesc(TII.get(AVM::MOV8Z));
    return constrain(I);
  case G_SEXT:
    I.setDesc(TII.get(AVM::MOV8S));
    return constrain(I);
  case G_TRUNC:
    I.setDesc(TII.get(COPY));
    I.getOperand(1).setSubReg(AVM::sub_lo8);
    return select(I);
  case G_FREEZE:
    I.setDesc(TII.get(COPY));
    return select(I);
  case G_PHI:
    I.setDesc(TII.get(PHI));
    for (MachineOperand &MO : I.operands())
      if (MO.isReg() && MO.getReg().isVirtual() &&
          !RBI.constrainGenericRegister(MO.getReg(), AVM::CGPR16RegClass, MRI))
        return false;
    return true;
  default:
    return false;
  }
}

InstructionSelector *
llvm::createAVMInstructionSelector(const AVMTargetMachine &,
                                   const AVMSubtarget &STI,
                                   const AVMRegisterBankInfo &RBI) {
  return new AVMInstructionSelector(STI, RBI);
}
