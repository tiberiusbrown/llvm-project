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
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "avm-byte-phis"

namespace {
// An induction state carried through a nested loop can affect allocation in
// its much hotter body. Avoid that risk when all normalization savings for
// the dependent byte web are outside the nested loop. This is deliberately
// structural: it does not yet try to predict spills or compare block counts.
static bool
isUnprofitableOuterInduction(PHINode *PN,
                             const SmallPtrSetImpl<Instruction *> &Candidates,
                             function_ref<bool(Instruction *)> IsWordUse,
                             LoopInfo &LI, ScalarEvolution &SE) {
  Loop *L = LI.getLoopFor(PN->getParent());
  if (!L || L->getHeader() != PN->getParent() || L->isInnermost())
    return false;
  const auto *AR = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(PN));
  if (!AR || AR->getLoop() != L || !AR->isAffine())
    return false;

  SmallVector<Instruction *, 8> Consumers;
  SmallVector<Instruction *, 8> Worklist{PN};
  SmallPtrSet<Instruction *, 16> Seen{PN};
  while (!Worklist.empty())
    for (User *U : Worklist.pop_back_val()->users()) {
      auto *I = cast<Instruction>(U);
      if (Candidates.contains(I) && Seen.insert(I).second)
        Worklist.push_back(I);
      if (IsWordUse(I))
        Consumers.push_back(I);
    }

  SmallPtrSet<BasicBlock *, 1> Excluded{L->getHeader()};
  for (Loop *Nested : L->getLoopsInPreorder()) {
    if (Nested == L || llvm::any_of(Consumers, [&](Instruction *I) {
          return Nested->contains(I);
        }))
      continue;
    // State is live across the nested loop if execution can still reach a
    // backedge carrying the next induction value. Excluding the outer header
    // keeps a later outer iteration from making an exit-only path look live.
    // This also covers updates computed before entering the nested loop.
    for (BasicBlock *Incoming : PN->blocks())
      if (L->contains(Incoming) &&
          isPotentiallyReachable(Nested->getHeader(), Incoming, &Excluded,
                                 nullptr, &LI))
        return true;
  }
  return false;
}

class AVMBytePHIs : public FunctionPass {
public:
  static char ID;
  AVMBytePHIs() : FunctionPass(ID) {
    initializeAVMBytePHIsPass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override {
    if (skipFunction(F))
      return false;

    auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    SmallVector<Instruction *, 16> Values;
    SmallPtrSet<Instruction *, 16> Candidates;
    DenseMap<BinaryOperator *, ConstantInt *> Decrements;
    for (BasicBlock &BB : F) {
      if (BB.getFirstInsertionPt() == BB.end())
        continue;
      for (PHINode &PN : BB.phis())
        if (PN.getType()->isIntegerTy(8)) {
          Values.push_back(&PN);
          Candidates.insert(&PN);
        }
    }

    // Follow arithmetic feeding the PHIs only when its unsigned byte result
    // equals the native word result. NUW flags carry the optimizer's range
    // proofs; ScalarEvolution can also prove bounded induction updates that
    // have not acquired flags. NSW alone does not exclude unsigned wrapping.
    for (unsigned I = 0; I != Values.size(); ++I)
      for (Value *V : Values[I]->operands()) {
        auto *Op = dyn_cast<BinaryOperator>(V);
        if (!Op || Candidates.contains(Op) ||
            (Op->getOpcode() != Instruction::Add &&
             Op->getOpcode() != Instruction::Sub))
          continue;
        if (!Op->hasNoUnsignedWrap() &&
            !SE.willNotOverflow(Op->getOpcode(), false,
                                SE.getSCEV(Op->getOperand(0)),
                                SE.getSCEV(Op->getOperand(1)), Op)) {
          // InstCombine canonicalizes byte subtraction by a constant to an
          // add of its negation. Prove the subtraction, then widen its positive
          // magnitude rather than zero-extending the negative byte constant.
          auto *Step = dyn_cast<ConstantInt>(Op->getOperand(1));
          if (Op->getOpcode() != Instruction::Add || !Step ||
              !Step->isNegative())
            continue;
          auto *Magnitude =
              ConstantInt::get(Step->getContext(), -Step->getValue());
          if (!SE.willNotOverflow(Instruction::Sub, false,
                                  SE.getSCEV(Op->getOperand(0)),
                                  SE.getSCEV(Magnitude), Op))
            continue;
          Decrements[Op] = Magnitude;
        }
        Values.push_back(Op);
        Candidates.insert(Op);
      }

    // Byte loads and constants already produce zero-extended native words.
    // Keep that representation through joins and proven nonwrapping updates,
    // including cycles. An unknown/wrapping input invalidates its dependents.
    auto CanExtend = [&](Value *V) {
      if (isa<ConstantInt>(V))
        return true;
      if (auto *I = dyn_cast<Instruction>(V); Candidates.contains(I))
        return true;
      auto *LI = dyn_cast<LoadInst>(V);
      return LI && LI->getType()->isIntegerTy(8) &&
             (LI->getPointerAddressSpace() == 0 ||
              LI->getPointerAddressSpace() == 1);
    };
    SmallVector<Instruction *, 16> Invalid;
    for (Instruction *I : Values)
      if (!llvm::all_of(I->operands(), CanExtend))
        Invalid.push_back(I);
    auto InvalidateDependents = [&] {
      while (!Invalid.empty()) {
        Instruction *I = Invalid.pop_back_val();
        if (!Candidates.erase(I))
          continue;
        for (User *U : I->users())
          if (auto *UserI = dyn_cast<Instruction>(U);
              Candidates.contains(UserI))
            Invalid.push_back(UserI);
      }
    };
    InvalidateDependents();

    // Do not widen arithmetic left behind by a rejected PHI graph.
    SmallPtrSet<Instruction *, 16> Needed;
    SmallVector<Instruction *, 16> Worklist;
    for (Instruction *I : Values)
      if (isa<PHINode>(I) && Candidates.contains(I)) {
        Needed.insert(I);
        Worklist.push_back(I);
      }
    while (!Worklist.empty())
      for (Value *V : Worklist.pop_back_val()->operands())
        if (auto *I = dyn_cast<Instruction>(V);
            Candidates.contains(I) && Needed.insert(I).second)
          Worklist.push_back(I);
    Candidates = std::move(Needed);
    if (Candidates.empty())
      return false;

    auto IsWordUse = [&](Instruction *I) {
      if (isa<ZExtInst>(I))
        return true;
      if (auto *Cmp = dyn_cast<ICmpInst>(I)) {
        if ((!Cmp->isUnsigned() && !Cmp->isEquality()) ||
            !CanExtend(Cmp->getOperand(0)) || !CanExtend(Cmp->getOperand(1)))
          return false;
        auto *LHS = dyn_cast<ConstantInt>(Cmp->getOperand(0));
        auto *RHS = dyn_cast<ConstantInt>(Cmp->getOperand(1));
        return (!LHS || !LHS->isZero()) && (!RHS || !RHS->isZero());
      }
      if (auto *Switch = dyn_cast<SwitchInst>(I))
        return llvm::any_of(Switch->cases(), [](auto Case) {
          return !Case.getCaseValue()->isZero();
        });
      return false;
    };

    // A rejected web can invalidate another web's unsigned comparison. Repeat
    // until only graphs with eligible hot consumers remain exempt.
    do {
      for (Instruction *I : Values)
        if (auto *PN = dyn_cast<PHINode>(I);
            PN && Candidates.contains(PN) &&
            isUnprofitableOuterInduction(PN, Candidates, IsWordUse, LI, SE))
          Invalid.push_back(PN);
      if (Invalid.empty())
        break;
      InvalidateDependents();
    } while (!Candidates.empty());

    // Widen only graphs with an unsigned consumer that would otherwise need
    // normalization. Byte stores and comparisons with zero already use the
    // low byte directly. Promoting their recurrences has no benefit and can
    // disturb scheduling and register allocation in surrounding loops.
    Needed.clear();
    for (Instruction *I : Values) {
      if (!Candidates.contains(I))
        continue;
      bool HasWordUse = llvm::any_of(
          I->users(), [&](User *U) { return IsWordUse(cast<Instruction>(U)); });
      if (HasWordUse) {
        Needed.insert(I);
        Worklist.push_back(I);
      }
    }
    while (!Worklist.empty())
      for (Value *V : Worklist.pop_back_val()->operands())
        if (auto *I = dyn_cast<Instruction>(V);
            Candidates.contains(I) && Needed.insert(I).second)
          Worklist.push_back(I);
    Candidates = std::move(Needed);
    if (Candidates.empty())
      return false;

    Type *WordTy = Type::getInt16Ty(F.getContext());
    DenseMap<Value *, Value *> Words;
    for (Instruction *I : Values)
      if (auto *PN = dyn_cast<PHINode>(I); PN && Candidates.contains(PN)) {
        auto *Wide =
            PHINode::Create(WordTy, PN->getNumIncomingValues(),
                            PN->getName() + ".word", PN->getIterator());
        Wide->setDebugLoc(PN->getDebugLoc());
        Words[PN] = Wide;
      }

    auto GetWord = [&](auto &&GetWord, Value *V) -> Value * {
      if (Value *Word = Words.lookup(V))
        return Word;
      if (auto *CI = dyn_cast<ConstantInt>(V))
        return ConstantInt::get(WordTy, CI->getZExtValue());
      if (auto *Op = dyn_cast<BinaryOperator>(V)) {
        Value *LHS = GetWord(GetWord, Op->getOperand(0));
        ConstantInt *Magnitude = Decrements.lookup(Op);
        Value *RHS =
            GetWord(GetWord, Magnitude ? Magnitude : Op->getOperand(1));
        IRBuilder<> B(Op);
        Value *Wide =
            B.CreateBinOp(Magnitude ? Instruction::Sub : Op->getOpcode(), LHS,
                          RHS, Op->getName() + ".word");
        if (auto *WideOp = dyn_cast<BinaryOperator>(Wide))
          WideOp->setHasNoUnsignedWrap(true);
        return Words[V] = Wide;
      }
      auto *LI = cast<LoadInst>(V);
      IRBuilder<> B(LI->getParent(), std::next(LI->getIterator()));
      B.SetCurrentDebugLocation(LI->getDebugLoc());
      return Words[V] = B.CreateZExt(V, WordTy, V->getName() + ".word");
    };
    for (Instruction *I : Values)
      if (auto *PN = dyn_cast<PHINode>(I); PN && Candidates.contains(PN)) {
        auto *Wide = cast<PHINode>(Words[PN]);
        for (unsigned I = 0; I != PN->getNumIncomingValues(); ++I)
          Wide->addIncoming(GetWord(GetWord, PN->getIncomingValue(I)),
                            PN->getIncomingBlock(I));
      }

    // Widen consumers that need the unsigned value. Other uses retain their
    // byte semantics through a truncation; memory widths and ABI types stay
    // unchanged. This avoids promoting unrelated arithmetic/comparison trees.
    SmallVector<Instruction *, 16> Dead;
    SmallPtrSet<Instruction *, 16> Seen;
    for (Instruction *V : Values) {
      if (!Candidates.contains(V))
        continue;
      SmallVector<User *, 8> Users(V->users());
      for (User *U : Users) {
        auto *I = cast<Instruction>(U);
        if (!Seen.insert(I).second)
          continue;
        IRBuilder<> B(I);
        if (auto *Ext = dyn_cast<ZExtInst>(I)) {
          Value *Word = Words[V];
          Ext->replaceAllUsesWith(B.CreateZExtOrTrunc(Word, Ext->getType()));
          Dead.push_back(Ext);
        } else if (auto *Cmp = dyn_cast<ICmpInst>(I)) {
          if ((!Cmp->isUnsigned() && !Cmp->isEquality()) ||
              !CanExtend(Cmp->getOperand(0)) || !CanExtend(Cmp->getOperand(1)))
            continue;
          // Byte tests against zero already lower directly to TST8. Keep
          // their byte type even when another consumer needs a native word.
          auto *LHS = dyn_cast<ConstantInt>(Cmp->getOperand(0));
          auto *RHS = dyn_cast<ConstantInt>(Cmp->getOperand(1));
          if ((LHS && LHS->isZero()) || (RHS && RHS->isZero()))
            continue;
          Value *WideCmp = B.CreateICmp(Cmp->getPredicate(),
                                        GetWord(GetWord, Cmp->getOperand(0)),
                                        GetWord(GetWord, Cmp->getOperand(1)));
          Cmp->replaceAllUsesWith(WideCmp);
          Dead.push_back(Cmp);
        } else if (auto *Switch = dyn_cast<SwitchInst>(I)) {
          auto *WideSwitch =
              SwitchInst::Create(Words[V], Switch->getDefaultDest(),
                                 Switch->getNumCases(), Switch->getIterator());
          WideSwitch->copyMetadata(*Switch);
          WideSwitch->setDebugLoc(Switch->getDebugLoc());
          for (auto Case : Switch->cases())
            WideSwitch->addCase(
                cast<ConstantInt>(GetWord(GetWord, Case.getCaseValue())),
                Case.getCaseSuccessor());
          Dead.push_back(Switch);
        }
      }
    }
    for (Instruction *I : Dead)
      I->eraseFromParent();

    SmallVector<Instruction *, 16> Truncs;
    for (Instruction *I : Values)
      if (Candidates.contains(I)) {
        IRBuilder<> B(I);
        if (isa<PHINode>(I))
          B.SetInsertPoint(I->getParent(),
                           I->getParent()->getFirstInsertionPt());
        Value *Trunc = B.CreateTrunc(Words[I], I->getType());
        I->replaceAllUsesWith(Trunc);
        if (auto *TruncI = dyn_cast<Instruction>(Trunc))
          Truncs.push_back(TruncI);
      }
    for (Instruction *I : Values)
      if (Candidates.contains(I))
        I->dropAllReferences();
    for (Instruction *I : Values)
      if (Candidates.contains(I))
        I->eraseFromParent();
    for (Instruction *I : Truncs)
      if (I->use_empty())
        I->eraseFromParent();
    return true;
  }
};
} // namespace

char AVMBytePHIs::ID = 0;
INITIALIZE_PASS_BEGIN(AVMBytePHIs, DEBUG_TYPE, "AVM byte PHI promotion", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_END(AVMBytePHIs, DEBUG_TYPE, "AVM byte PHI promotion", false,
                    false)
FunctionPass *llvm::createAVMBytePHIsPass() { return new AVMBytePHIs(); }
