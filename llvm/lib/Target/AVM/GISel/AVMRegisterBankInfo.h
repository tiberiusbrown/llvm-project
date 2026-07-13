#ifndef LLVM_LIB_TARGET_AVM_GISEL_AVMREGISTERBANKINFO_H
#define LLVM_LIB_TARGET_AVM_GISEL_AVMREGISTERBANKINFO_H

#include "../MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/RegisterBankInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGBANK_DECLARATIONS
#include "AVMGenRegisterBank.inc"

namespace llvm {
class TargetRegisterInfo;

class AVMGenRegisterBankInfo : public RegisterBankInfo {
protected:
#define GET_TARGET_REGBANK_CLASS
#include "AVMGenRegisterBank.inc"
};

class AVMRegisterBankInfo final : public AVMGenRegisterBankInfo {
public:
  explicit AVMRegisterBankInfo(const TargetRegisterInfo &TRI);
  const InstructionMapping &
  getInstrMapping(const MachineInstr &MI) const override;
};
} // namespace llvm

#endif
