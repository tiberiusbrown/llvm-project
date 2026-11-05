//===-- AVMAtomicLibcalls.cpp - Runtime helpers for eight-byte atomics -----===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// AVM's runtime provides the eight-byte atomic operations as ordinary functions named
// __atomic_*_8 (builtins/atomic.c). LLVM calls them only for accesses whose alignment is at
// least the access size. AVM gives every type byte alignment, so the expansion in
// AtomicExpandPass falls back to generic helpers and compare-and-swap loops. Those loops need
// large stack frames at -O0, and the generic helpers copy through temporaries. This pass
// calls the sized helpers directly, before AtomicExpandPass runs. The helpers accept any
// address, so the byte alignment of the object does not matter.
//
// Only accesses in address space 0 are replaced. Operations that have no helper, such as
// minimum and maximum, stay with LLVM's expansion.
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "avm-atomic-libcalls"
#define PASS_NAME "AVM eight-byte atomic runtime helpers"

namespace {

// Returns the name of the helper that implements an atomic read-modify-write operation on
// eight bytes, or null when the runtime has no helper for it.
const char *rmwHelperName(AtomicRMWInst::BinOp Op) {
  switch (Op) {
  case AtomicRMWInst::Xchg:
    return "__atomic_exchange_8";
  case AtomicRMWInst::Add:
    return "__atomic_fetch_add_8";
  case AtomicRMWInst::Sub:
    return "__atomic_fetch_sub_8";
  case AtomicRMWInst::And:
    return "__atomic_fetch_and_8";
  case AtomicRMWInst::Or:
    return "__atomic_fetch_or_8";
  case AtomicRMWInst::Xor:
    return "__atomic_fetch_xor_8";
  case AtomicRMWInst::Nand:
    return "__atomic_fetch_nand_8";
  default:
    return nullptr;
  }
}

bool isEightByteAccess(const Value *Pointer, Type *Ty) {
  return Ty->isIntegerTy(64) && Pointer->getType()->getPointerAddressSpace() == 0;
}

// The helpers take the memory order as a C int. On AVM that is sixteen bits, so the argument
// has the same width. The runtime ignores the value, because the single-threaded model has no
// ordering to enforce, so it only documents the request.
Value *orderArgument(IRBuilder<> &B, AtomicOrdering Order) {
  return B.getInt16(static_cast<uint16_t>(Order));
}

// Calls a runtime helper with the given signature, creating the declaration when needed.
CallInst *callHelper(IRBuilder<> &B, Module &M, StringRef Name, Type *Result,
                     ArrayRef<Value *> Args) {
  SmallVector<Type *, 4> ParamTypes;
  for (Value *Arg : Args)
    ParamTypes.push_back(Arg->getType());
  FunctionCallee Helper =
      M.getOrInsertFunction(Name, FunctionType::get(Result, ParamTypes, false));
  return B.CreateCall(Helper, Args);
}

class AVMAtomicLibcalls final : public FunctionPass {
public:
  static char ID;

  AVMAtomicLibcalls() : FunctionPass(ID) {
    initializeAVMAtomicLibcallsPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override;

private:
  void lowerLoad(LoadInst &LI);
  void lowerStore(StoreInst &SI);
  void lowerRMW(AtomicRMWInst &RMW);
  void lowerCmpXchg(AtomicCmpXchgInst &CAS, Value *ExpectedSlot);
};

void AVMAtomicLibcalls::lowerLoad(LoadInst &LI) {
  IRBuilder<> B(&LI);
  CallInst *Call = callHelper(B, *LI.getModule(), "__atomic_load_8", B.getInt64Ty(),
                              {LI.getPointerOperand(), orderArgument(B, LI.getOrdering())});
  LI.replaceAllUsesWith(Call);
  LI.eraseFromParent();
}

void AVMAtomicLibcalls::lowerStore(StoreInst &SI) {
  IRBuilder<> B(&SI);
  callHelper(B, *SI.getModule(), "__atomic_store_8", B.getVoidTy(),
             {SI.getPointerOperand(), SI.getValueOperand(), orderArgument(B, SI.getOrdering())});
  SI.eraseFromParent();
}

void AVMAtomicLibcalls::lowerRMW(AtomicRMWInst &RMW) {
  IRBuilder<> B(&RMW);
  CallInst *Call =
      callHelper(B, *RMW.getModule(), rmwHelperName(RMW.getOperation()), B.getInt64Ty(),
                 {RMW.getPointerOperand(), RMW.getValOperand(), orderArgument(B, RMW.getOrdering())});
  RMW.replaceAllUsesWith(Call);
  RMW.eraseFromParent();
}

void AVMAtomicLibcalls::lowerCmpXchg(AtomicCmpXchgInst &CAS, Value *ExpectedSlot) {
  IRBuilder<> B(&CAS);
  B.CreateStore(CAS.getCompareOperand(), ExpectedSlot);

  // int __atomic_compare_exchange_8(void *ptr, void *expected, uint64_t desired, int, int).
  // The helper returns 1 on success and 0 on failure, as a byte-sized result, so the call
  // returns an integer and the success flag is a comparison with zero. On success the helper
  // leaves the expected value unchanged. On failure it stores the current value through the
  // expected pointer, so the slot holds the value that cmpxchg returns in both cases.
  CallInst *Status = callHelper(
      B, *CAS.getModule(), "__atomic_compare_exchange_8", B.getInt8Ty(),
      {CAS.getPointerOperand(), ExpectedSlot, CAS.getNewValOperand(),
       orderArgument(B, CAS.getSuccessOrdering()), orderArgument(B, CAS.getFailureOrdering())});
  Value *Success = B.CreateICmpNE(Status, B.getInt8(0));

  Value *Old = B.CreateLoad(B.getInt64Ty(), ExpectedSlot);

  // Clang's compare-exchange code reads the two fields with extractvalue. Replacing those
  // directly keeps the aggregate out of phi nodes, which -O0 would spill once per predecessor.
  // Any other use still receives the rebuilt aggregate.
  SmallVector<ExtractValueInst *, 2> Extracts;
  bool HasOtherUses = false;
  for (User *U : CAS.users()) {
    auto *EV = dyn_cast<ExtractValueInst>(U);
    if (EV && EV->getNumIndices() == 1 && EV->getIndices()[0] < 2)
      Extracts.push_back(EV);
    else
      HasOtherUses = true;
  }
  for (ExtractValueInst *EV : Extracts) {
    EV->replaceAllUsesWith(EV->getIndices()[0] == 0 ? Old : Success);
    EV->eraseFromParent();
  }
  if (HasOtherUses) {
    Value *Result = B.CreateInsertValue(PoisonValue::get(CAS.getType()), Old, 0);
    Result = B.CreateInsertValue(Result, Success, 1);
    CAS.replaceAllUsesWith(Result);
  }
  CAS.eraseFromParent();
}

bool AVMAtomicLibcalls::runOnFunction(Function &F) {
  // Collect first, because lowering replaces instructions in place.
  SmallVector<Instruction *, 8> Work;
  for (Instruction &I : instructions(F)) {
    if (auto *LI = dyn_cast<LoadInst>(&I)) {
      if (LI->isAtomic() && isEightByteAccess(LI->getPointerOperand(), LI->getType()))
        Work.push_back(LI);
    } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
      if (SI->isAtomic() &&
          isEightByteAccess(SI->getPointerOperand(), SI->getValueOperand()->getType()))
        Work.push_back(SI);
    } else if (auto *RMW = dyn_cast<AtomicRMWInst>(&I)) {
      if (rmwHelperName(RMW->getOperation()) &&
          isEightByteAccess(RMW->getPointerOperand(), RMW->getValOperand()->getType()))
        Work.push_back(RMW);
    } else if (auto *CAS = dyn_cast<AtomicCmpXchgInst>(&I)) {
      if (isEightByteAccess(CAS->getPointerOperand(), CAS->getCompareOperand()->getType()))
        Work.push_back(CAS);
    }
  }
  if (Work.empty())
    return false;

  // One expected-value slot serves every compare-exchange in the function. The slot lives in
  // the entry block, and each use stores the expected value before the helper reads it.
  Value *ExpectedSlot = nullptr;
  for (Instruction *I : Work) {
    if (auto *LI = dyn_cast<LoadInst>(I)) {
      lowerLoad(*LI);
    } else if (auto *SI = dyn_cast<StoreInst>(I)) {
      lowerStore(*SI);
    } else if (auto *RMW = dyn_cast<AtomicRMWInst>(I)) {
      lowerRMW(*RMW);
    } else {
      if (!ExpectedSlot) {
        IRBuilder<> EntryBuilder(&*F.getEntryBlock().getFirstInsertionPt());
        AllocaInst *Slot =
            EntryBuilder.CreateAlloca(EntryBuilder.getInt64Ty(), nullptr, "avm.atomic.expected");
        Slot->setAlignment(Align(1));
        ExpectedSlot = Slot;
      }
      lowerCmpXchg(*cast<AtomicCmpXchgInst>(I), ExpectedSlot);
    }
  }
  return true;
}

} // namespace

char AVMAtomicLibcalls::ID = 0;

INITIALIZE_PASS(AVMAtomicLibcalls, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMAtomicLibcallsPass() {
  return new AVMAtomicLibcalls();
}
