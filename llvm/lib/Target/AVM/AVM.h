//===-- AVM.h - Top-level AVM backend interface -------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVM_H
#define LLVM_LIB_TARGET_AVM_AVM_H

#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class AVMTargetMachine;
class FunctionPass;
class PassRegistry;

FunctionPass *createAVMISelDag(AVMTargetMachine &TM, CodeGenOptLevel OptLevel);
FunctionPass *createAVMExpandPseudoPass();

void initializeAVMAsmPrinterPass(PassRegistry &);
void initializeAVMDAGToDAGISelLegacyPass(PassRegistry &);
void initializeAVMExpandPseudoPass(PassRegistry &);
} // namespace llvm

#endif
