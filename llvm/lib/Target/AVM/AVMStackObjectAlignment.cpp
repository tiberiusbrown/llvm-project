//===-- AVMStackObjectAlignment.cpp - Byte-aligned stack objects -----------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// AVM's stack pointer has no alignment. Frames sit at arbitrary byte addresses, and every AVM
// type has byte alignment. An alloca that asks for more alignment makes SelectionDAG assume that
// the low address bits of the object are zero, and it then lowers `sp + offset` as `sp | offset`.
// That is wrong whenever sp is not aligned, so a copy into a padded temporary wrote the wrong
// bytes. This pass gives every alloca byte alignment, which is the only alignment the stack can
// guarantee.
//
// The pass runs after the optimizer, because InstCombine would raise the alignment again.
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "avm-stack-object-alignment"
#define PASS_NAME "AVM byte-aligned stack objects"

namespace {

class AVMStackObjectAlignment final : public FunctionPass {
public:
  static char ID;

  AVMStackObjectAlignment() : FunctionPass(ID) {
    initializeAVMStackObjectAlignmentPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
    bool Changed = false;
    for (Instruction &I : instructions(F)) {
      auto *AI = dyn_cast<AllocaInst>(&I);
      if (AI && AI->getAlign().value() > 1) {
        AI->setAlignment(Align(1));
        Changed = true;
      }
    }
    return Changed;
  }
};

} // namespace

char AVMStackObjectAlignment::ID = 0;

INITIALIZE_PASS(AVMStackObjectAlignment, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMStackObjectAlignmentPass() {
  return new AVMStackObjectAlignment();
}
