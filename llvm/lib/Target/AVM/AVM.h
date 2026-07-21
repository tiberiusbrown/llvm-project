//===-- AVM.h - Top-level AVM backend interface -------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVM_H
#define LLVM_LIB_TARGET_AVM_AVM_H

#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class AVMTargetMachine;
class FunctionPass;
class PassRegistry;

namespace AVMCC {
enum CondCode : unsigned { EQ, NE, ULT, UGE, SLT, SGE };
}

FunctionPass *createAVMISelDag(AVMTargetMachine &TM, CodeGenOptLevel OptLevel);
FunctionPass *createAVMBranchPolarityPass();
FunctionPass *createAVMExpandPseudoPass();
FunctionPass *createAVMFinalControlFlowPass();
FunctionPass *createAVMServiceResultPass();

void initializeAVMAsmPrinterPass(PassRegistry &);
void initializeAVMBranchPolarityPass(PassRegistry &);
void initializeAVMDAGToDAGISelLegacyPass(PassRegistry &);
void initializeAVMExpandPseudoPass(PassRegistry &);
void initializeAVMFinalControlFlowPass(PassRegistry &);
void initializeAVMServiceResultPass(PassRegistry &);
} // namespace llvm

#endif
