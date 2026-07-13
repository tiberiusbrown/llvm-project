#ifndef LLVM_LIB_TARGET_AVM_GISEL_AVMINSTRUCTIONSELECTOR_H
#define LLVM_LIB_TARGET_AVM_GISEL_AVMINSTRUCTIONSELECTOR_H

namespace llvm {
class AVMRegisterBankInfo;
class AVMSubtarget;
class AVMTargetMachine;
class InstructionSelector;

InstructionSelector *
createAVMInstructionSelector(const AVMTargetMachine &TM,
                             const AVMSubtarget &STI,
                             const AVMRegisterBankInfo &RBI);
} // namespace llvm

#endif
