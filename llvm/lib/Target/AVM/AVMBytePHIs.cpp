//===-- AVMBytePHIs.cpp - Preserve byte facts through PHIs ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "avm-byte-phis"

namespace {
class AVMBytePHIs : public FunctionPass {
public:
  static char ID;
  AVMBytePHIs() : FunctionPass(ID) {
    initializeAVMBytePHIsPass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override {
    if (skipFunction(F))
      return false;

    SmallVector<PHINode *, 16> Phis;
    SmallPtrSet<PHINode *, 16> Candidates;
    for (BasicBlock &BB : F) {
      if (BB.getFirstInsertionPt() == BB.end())
        continue;
      for (PHINode &PN : BB.phis())
        if (PN.getType()->isIntegerTy(8)) {
          Phis.push_back(&PN);
          Candidates.insert(&PN);
        }
    }

    // Byte loads and constants already produce zero-extended native words.
    // Keep that representation through joins, including cyclic PHI graphs.
    // Arithmetic inputs are excluded: extending a wrapping byte operation
    // could require an additional instruction or change its semantics.
    auto CanExtend = [&](Value *V) {
      if (isa<ConstantInt>(V))
        return true;
      if (auto *PN = dyn_cast<PHINode>(V))
        return Candidates.contains(PN);
      auto *LI = dyn_cast<LoadInst>(V);
      return LI && LI->getType()->isIntegerTy(8) &&
             (LI->getPointerAddressSpace() == 0 ||
              LI->getPointerAddressSpace() == 1);
    };
    SmallVector<PHINode *, 16> Invalid;
    for (PHINode *PN : Phis)
      if (!llvm::all_of(PN->incoming_values(), CanExtend))
        Invalid.push_back(PN);
    while (!Invalid.empty()) {
      PHINode *PN = Invalid.pop_back_val();
      if (!Candidates.erase(PN))
        continue;
      for (User *U : PN->users())
        if (auto *UserPN = dyn_cast<PHINode>(U))
          Invalid.push_back(UserPN);
    }
    if (Candidates.empty())
      return false;

    Type *WordTy = Type::getInt16Ty(F.getContext());
    DenseMap<Value *, Value *> Words;
    for (PHINode *PN : Phis)
      if (Candidates.contains(PN)) {
        auto *Wide = PHINode::Create(WordTy, PN->getNumIncomingValues(),
                                    PN->getName() + ".word", PN->getIterator());
        Wide->setDebugLoc(PN->getDebugLoc());
        Words[PN] = Wide;
      }

    auto GetWord = [&](Value *V) -> Value * {
      if (Value *Word = Words.lookup(V))
        return Word;
      if (auto *CI = dyn_cast<ConstantInt>(V))
        return ConstantInt::get(WordTy, CI->getZExtValue());
      auto *LI = cast<LoadInst>(V);
      IRBuilder<> B(LI->getParent(), std::next(LI->getIterator()));
      B.SetCurrentDebugLocation(LI->getDebugLoc());
      return Words[V] = B.CreateZExt(V, WordTy, V->getName() + ".word");
    };
    for (PHINode *PN : Phis)
      if (Candidates.contains(PN)) {
        auto *Wide = cast<PHINode>(Words[PN]);
        for (unsigned I = 0; I != PN->getNumIncomingValues(); ++I)
          Wide->addIncoming(GetWord(PN->getIncomingValue(I)),
                            PN->getIncomingBlock(I));
      }

    // Widen consumers that need the unsigned value. Other uses retain their
    // byte semantics through a truncation; memory widths and ABI types stay
    // unchanged. This avoids promoting unrelated arithmetic/comparison trees.
    SmallVector<Instruction *, 16> Dead;
    SmallPtrSet<Instruction *, 16> Seen;
    for (PHINode *PN : Phis) {
      if (!Candidates.contains(PN))
        continue;
      SmallVector<User *, 8> Users(PN->users());
      for (User *U : Users) {
        auto *I = cast<Instruction>(U);
        if (!Seen.insert(I).second)
          continue;
        IRBuilder<> B(I);
        if (auto *Ext = dyn_cast<ZExtInst>(I)) {
          Value *Word = Words[PN];
          Ext->replaceAllUsesWith(B.CreateZExtOrTrunc(Word, Ext->getType()));
          Dead.push_back(Ext);
        } else if (auto *Cmp = dyn_cast<ICmpInst>(I)) {
          if ((!Cmp->isUnsigned() && !Cmp->isEquality()) ||
              !CanExtend(Cmp->getOperand(0)) ||
              !CanExtend(Cmp->getOperand(1)))
            continue;
          Value *WideCmp = B.CreateICmp(Cmp->getPredicate(),
                                       GetWord(Cmp->getOperand(0)),
                                       GetWord(Cmp->getOperand(1)));
          Cmp->replaceAllUsesWith(WideCmp);
          Dead.push_back(Cmp);
        } else if (auto *Switch = dyn_cast<SwitchInst>(I)) {
          auto *WideSwitch = SwitchInst::Create(
              Words[PN], Switch->getDefaultDest(), Switch->getNumCases(),
              Switch->getIterator());
          WideSwitch->copyMetadata(*Switch);
          WideSwitch->setDebugLoc(Switch->getDebugLoc());
          for (auto Case : Switch->cases())
            WideSwitch->addCase(
                cast<ConstantInt>(GetWord(Case.getCaseValue())),
                Case.getCaseSuccessor());
          Dead.push_back(Switch);
        }
      }
    }
    for (Instruction *I : Dead)
      I->eraseFromParent();

    SmallVector<Instruction *, 16> Truncs;
    for (PHINode *PN : Phis)
      if (Candidates.contains(PN)) {
        IRBuilder<> B(PN->getParent(), PN->getParent()->getFirstInsertionPt());
        B.SetCurrentDebugLocation(PN->getDebugLoc());
        auto *Trunc = cast<Instruction>(B.CreateTrunc(Words[PN], PN->getType()));
        PN->replaceAllUsesWith(Trunc);
        Truncs.push_back(Trunc);
      }
    for (PHINode *PN : Phis)
      if (Candidates.contains(PN))
        PN->eraseFromParent();
    for (Instruction *I : Truncs)
      if (I->use_empty())
        I->eraseFromParent();
    return true;
  }
};
} // namespace

char AVMBytePHIs::ID = 0;
INITIALIZE_PASS(AVMBytePHIs, DEBUG_TYPE, "AVM byte PHI promotion", false, false)
FunctionPass *llvm::createAVMBytePHIsPass() { return new AVMBytePHIs(); }
