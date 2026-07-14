//===-- ABC.h - Top-level interface for ABC representation ------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_ABC_H
#define LLVM_LIB_TARGET_ABC_ABC_H

#include "MCTargetDesc/ABCMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class ABCSubtarget;
class ABCTargetMachine;
class FunctionPass;
class PassRegistry;

void initializeABCAsmPrinterPass(PassRegistry &);
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_ABC_H
