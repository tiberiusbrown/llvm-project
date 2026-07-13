#include "AVMRegisterBankInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

#define GET_TARGET_REGBANK_IMPL
#include "AVMGenRegisterBank.inc"

using namespace llvm;

AVMRegisterBankInfo::AVMRegisterBankInfo(const TargetRegisterInfo &)
    : AVMGenRegisterBankInfo() {}

const RegisterBankInfo::InstructionMapping &
AVMRegisterBankInfo::getInstrMapping(const MachineInstr &MI) const {
  const InstructionMapping &Default = getInstrMappingImpl(MI);
  if (Default.isValid())
    return Default;

  const MachineRegisterInfo &MRI = MI.getMF()->getRegInfo();
  SmallVector<const ValueMapping *, 8> Mappings;
  for (const MachineOperand &MO : MI.operands()) {
    if (!MO.isReg() || !MO.getReg()) {
      Mappings.push_back(nullptr);
      continue;
    }
    unsigned Bits =
        MO.getReg().isVirtual() ? MRI.getType(MO.getReg()).getSizeInBits() : 16;
    Mappings.push_back(
        &getValueMapping(0, std::max(1u, Bits), AVM::GPRRegBank));
  }
  const ValueMapping *Operands = getOperandsMapping(Mappings);
  return getInstructionMapping(DefaultMappingID, 1, Operands,
                               MI.getNumOperands());
}
