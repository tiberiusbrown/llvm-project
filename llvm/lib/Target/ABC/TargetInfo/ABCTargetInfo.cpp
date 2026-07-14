//===-- ABCTargetInfo.cpp - ABC target implementation ---------------------===//

#include "TargetInfo/ABCTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

Target &llvm::getTheABCTarget() {
  static Target TheABCTarget;
  return TheABCTarget;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeABCTargetInfo() {
  RegisterTarget<Triple::abc, /*HasJIT=*/false> X(
      getTheABCTarget(), "abc", "ABC bytecode VM", "ABC");
}
