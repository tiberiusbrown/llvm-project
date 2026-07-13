#ifndef LLVM_LIB_TARGET_AVM_GISEL_AVMCALLLOWERING_H
#define LLVM_LIB_TARGET_AVM_GISEL_AVMCALLLOWERING_H

#include "llvm/CodeGen/GlobalISel/CallLowering.h"

namespace llvm {
class AVMTargetLowering;

class AVMCallLowering final : public CallLowering {
public:
  explicit AVMCallLowering(const AVMTargetLowering &TLI);

  bool lowerReturn(MachineIRBuilder &MIRBuilder, const Value *Val,
                   ArrayRef<Register> VRegs, FunctionLoweringInfo &FLI,
                   Register SwiftErrorVReg) const override;
  bool lowerFormalArguments(MachineIRBuilder &MIRBuilder, const Function &F,
                            ArrayRef<ArrayRef<Register>> VRegs,
                            FunctionLoweringInfo &FLI) const override;
  bool lowerCall(MachineIRBuilder &MIRBuilder,
                 CallLoweringInfo &Info) const override;
};
} // namespace llvm

#endif
