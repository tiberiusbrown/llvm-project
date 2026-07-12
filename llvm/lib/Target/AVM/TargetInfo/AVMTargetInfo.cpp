#include "AVMTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

Target &llvm::getTheAVMTarget() {
  static Target TheAVMTarget;
  return TheAVMTarget;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVMTargetInfo() {
  RegisterTarget<Triple::avm, false> X(getTheAVMTarget(), "avm",
                                       "Arduboy Virtual Machine", "AVM");
}
