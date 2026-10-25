//===-- AVMAggregateArguments.cpp - Read-only aggregate ABI ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Replace internal byval parameters with borrowed pointers before SelectionDAG
// constructs outgoing copies. Read-only use of the parameter alone is not
// sufficient: a write through another argument, global, or callback could change
// the original while the callee expects a snapshot. Require the whole callee to
// be read-only and nosync, and prove that the parameter's identity cannot escape.
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

#define DEBUG_TYPE "avm-aggregate-arguments"
#define PASS_NAME "AVM internal read-only aggregate argument specialization"

static cl::opt<bool> DisableAggregateArguments(
    "avm-disable-aggregate-arguments", cl::Hidden, cl::init(false),
    cl::desc("Disable AVM internal read-only aggregate argument specialization"));

namespace {
// Only bounded, non-volatile, non-atomic loads may observe the private copy.
// In particular, even a readonly nocapture call could compare its address with
// an alias of the original. Do not let such a use observe the borrowed pointer.
static bool hasOnlyLoads(Argument &Arg, uint64_t Size, const DataLayout &DL) {
  SmallVector<std::pair<Value *, uint64_t>, 16> Worklist{{&Arg, 0}};
  SmallPtrSet<Value *, 32> Visited;
  while (!Worklist.empty()) {
    auto [Ptr, Offset] = Worklist.pop_back_val();
    if (!Visited.insert(Ptr).second)
      continue;
    for (User *U : Ptr->users()) {
      if (auto *GEP = dyn_cast<GetElementPtrInst>(U)) {
        APInt Delta(DL.getIndexTypeSizeInBits(Ptr->getType()), 0);
        if (!GEP->accumulateConstantOffset(DL, Delta) || Delta.isNegative() ||
            Delta.getLimitedValue() > Size - Offset)
          return false;
        Worklist.emplace_back(GEP, Offset + Delta.getZExtValue());
        continue;
      }
      if (auto *Cast = dyn_cast<BitCastInst>(U)) {
        Worklist.emplace_back(Cast, Offset);
        continue;
      }
      auto *Load = dyn_cast<LoadInst>(U);
      if (!Load || !Load->isSimple())
        return false;
      TypeSize LoadSize = DL.getTypeStoreSize(Load->getType());
      if (LoadSize.isScalable() || LoadSize.getFixedValue() > Size - Offset)
        return false;
    }
  }
  return true;
}

static bool specialize(Function &F) {
  if (!F.hasLocalLinkage() || F.isDeclaration() || F.isVarArg() ||
      F.hasFnAttribute(Attribute::OptimizeNone) || F.hasAddressTaken() ||
      F.hasMetadata(LLVMContext::MD_callback) || F.use_empty() ||
      !F.onlyReadsMemory() || !F.hasFnAttribute(Attribute::NoSync))
    return false;
  // An outgoing musttail call also constrains this function's incoming ABI,
  // even if the candidate parameter itself is not forwarded to that call.
  for (Instruction &I : instructions(F))
    if (auto *Call = dyn_cast<CallInst>(&I); Call && Call->isMustTailCall())
      return false;

  SmallVector<CallInst *, 4> Calls;
  for (User *U : F.users()) {
    auto *Call = dyn_cast<CallInst>(U);
    if (!Call || Call->getCalledOperand() != &F ||
        Call->getCallingConv() != F.getCallingConv() ||
        Call->arg_size() != F.arg_size() || Call->isMustTailCall() ||
        Call->hasOperandBundles())
      return false;
    Calls.push_back(Call);
  }

  SmallVector<Argument *, 4> Borrowed;
  const DataLayout &DL = F.getDataLayout();
  for (Argument &Arg : F.args()) {
    if (!Arg.hasByValAttr() || Arg.getType()->getPointerAddressSpace() != 0 ||
        Arg.hasAttribute(Attribute::NoAlias))
      continue;
    Type *Ty = Arg.getParamByValType();
    TypeSize Size = DL.getTypeAllocSize(Ty);
    if (Size.isScalable() || Size.getFixedValue() <= 4 ||
        !hasOnlyLoads(Arg, Size.getFixedValue(), DL))
      continue;
    unsigned I = Arg.getArgNo();
    // AVM's ABI alignment is one. Preserve stronger explicit alignments only
    // when every source is already guaranteed to satisfy them at the call.
    Align Alignment = Arg.getParamAlign().valueOrOne();
    if (!all_of(Calls, [&](CallInst *Call) {
          return Call->getAttributes().getParamByValType(I) == Ty &&
                 Call->getParamAlign(I).valueOrOne() >= Alignment &&
                 !Call->paramHasAttr(I, Attribute::NoAlias);
        }))
      continue;
    Borrowed.push_back(&Arg);
  }
  if (Borrowed.empty())
    return false;

  // The IR pointer types do not change, so all direct callers can adopt the
  // internal convention together without cloning the body or adding a wrapper.
  for (Argument *Arg : Borrowed) {
    unsigned I = Arg->getArgNo();
    F.removeParamAttr(I, Attribute::ByVal);
    F.removeParamAttr(I, Attribute::Writable);
    F.addParamAttr(I, Attribute::ReadOnly);
    for (CallInst *Call : Calls) {
      Call->removeParamAttr(I, Attribute::ByVal);
      Call->removeParamAttr(I, Attribute::Writable);
      Call->addParamAttr(I, Attribute::ReadOnly);
    }
  }
  // Explicit memory(none) attributes may previously have excluded private
  // byval reads. Those reads now refer to the caller's argument memory.
  MemoryEffects Reads = MemoryEffects::argMemOnly(ModRefInfo::Ref);
  F.setMemoryEffects(F.getMemoryEffects() | Reads);
  for (CallInst *Call : Calls) {
    Call->setMemoryEffects(Call->getMemoryEffects() | Reads);
    // A borrowed pointer may refer to this caller's frame. A former byval
    // tail call could copy it before discarding that frame; it must now stay.
    if (Call->isTailCall())
      Call->setTailCallKind(CallInst::TCK_None);
  }
  return true;
}

class AVMAggregateArguments final : public ModulePass {
public:
  static char ID;
  AVMAggregateArguments() : ModulePass(ID) {
    initializeAVMAggregateArgumentsPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) override {
    if (DisableAggregateArguments)
      return false;
    bool Changed = false;
    for (Function &F : M)
      Changed |= specialize(F);
    return Changed;
  }
};
} // namespace

char AVMAggregateArguments::ID = 0;
INITIALIZE_PASS(AVMAggregateArguments, DEBUG_TYPE, PASS_NAME, false, false)

ModulePass *llvm::createAVMAggregateArgumentsPass() {
  return new AVMAggregateArguments();
}
