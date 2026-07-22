//===-- AVMProgramMemoryWidening.cpp - Widen program loads ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"

using namespace llvm;

#define DEBUG_TYPE "avm-program-memory-widening"
#define PASS_NAME "AVM program-memory load widening"

static cl::opt<bool> DisableAVMProgramMemoryWidening(
    "avm-disable-program-memory-widening", cl::Hidden, cl::init(false),
    cl::desc("Disable AVM program-memory load widening"));

STATISTIC(NumByteLoopsUnrolled,
          "Number of AVM program-memory byte loops unrolled");
STATISTIC(NumWordLoopsUnrolled,
          "Number of AVM program-memory word loops unrolled");
STATISTIC(NumLoadsWidenedToI16,
          "Number of AVM program-memory load pairs widened to i16");
STATISTIC(NumLoadsWidenedToI32,
          "Number of AVM program-memory load pairs widened to i32");

namespace {

struct ProgramLoadAddress {
  LoadInst *Load;
  Value *Base;
  int64_t Offset;
};

static LoadInst *findSingleProgramStreamLoad(Loop *L, unsigned ElementBits,
                                             ScalarEvolution &SE,
                                             DominatorTree &DT) {
  if (!L->isInnermost() || L->getNumBlocks() != 1 || !L->isLoopSimplifyForm() ||
      !L->isLCSSAForm(DT) || !L->getLoopPreheader() || !L->getLoopLatch() ||
      !L->getExitingBlock() || !L->getUniqueExitBlock() ||
      hasUnrollTransformation(L) != TM_Unspecified)
    return nullptr;

  if (SE.getBackedgeTakenCount(L) == SE.getCouldNotCompute())
    return nullptr;

  if (unsigned TripCount = SE.getSmallConstantTripCount(L);
      TripCount != 0 && TripCount < 2)
    return nullptr;

  LoadInst *Candidate = nullptr;
  for (Instruction &I : *L->getHeader()) {
    if (I.mayReadOrWriteMemory()) {
      if (Candidate)
        return nullptr;

      auto *Load = dyn_cast<LoadInst>(&I);
      if (!Load || !Load->isSimple() || Load->getPointerAddressSpace() != 1 ||
          !Load->getType()->isIntegerTy(ElementBits))
        return nullptr;
      Candidate = Load;
      continue;
    }

    if (isa<CallBase>(I) || isa<FenceInst>(I) || I.mayHaveSideEffects())
      return nullptr;
  }

  if (!Candidate)
    return nullptr;

  const SCEV *PointerSCEV = SE.getSCEV(Candidate->getPointerOperand());
  const auto *AddRec = dyn_cast<SCEVAddRecExpr>(PointerSCEV);
  if (!AddRec || AddRec->getLoop() != L || !AddRec->isAffine())
    return nullptr;

  const auto *Step = dyn_cast<SCEVConstant>(AddRec->getStepRecurrence(SE));
  if (!Step || !Step->getAPInt().isSignedIntN(64))
    return nullptr;

  int64_t StepBytes = Step->getAPInt().sextOrTrunc(64).getSExtValue();
  if (StepBytes != static_cast<int64_t>(ElementBits / 8))
    return nullptr;

  return Candidate;
}

static LoopUnrollResult unrollByTwo(Loop *L, LoopInfo &LI, ScalarEvolution &SE,
                                    DominatorTree &DT, AssumptionCache &AC,
                                    TargetTransformInfo &TTI,
                                    OptimizationRemarkEmitter &ORE,
                                    Loop **RemainderLoop) {
  UnrollLoopOptions ULO{
      /*Count=*/2,
      /*Force=*/true,
      /*Runtime=*/true,
      /*AllowExpensiveTripCount=*/false,
      /*UnrollRemainder=*/false,
      /*ForgetAllSCEV=*/false,
      /*Heart=*/nullptr,
      /*SCEVExpansionBudget=*/16,
      /*RuntimeUnrollMultiExit=*/false,
      /*AddAdditionalAccumulators=*/false,
  };

  return UnrollLoop(L, ULO, &LI, &SE, &DT, &AC, &TTI, &ORE,
                    /*PreserveLCSSA=*/true, RemainderLoop);
}

static bool decomposeProgramPointer(Value *Pointer, const DataLayout &DL,
                                    Value *&Base, int64_t &Offset) {
  if (!Pointer->getType()->isPointerTy() ||
      Pointer->getType()->getPointerAddressSpace() != 1)
    return false;

  APInt OffsetAP(DL.getIndexTypeSizeInBits(Pointer->getType()), 0,
                 /*isSigned=*/true);
  Base = Pointer->stripAndAccumulateConstantOffsets(DL, OffsetAP,
                                                    /*AllowNonInbounds=*/true);
  if (!OffsetAP.isSignedIntN(64))
    return false;

  Offset = OffsetAP.sextOrTrunc(64).getSExtValue();
  return true;
}

static bool combineProgramLoadPair(LoadInst *First, unsigned ElementBits,
                                   const DataLayout &DL) {
  if ((ElementBits != 8 && ElementBits != 16) || !First->isSimple() ||
      First->getPointerAddressSpace() != 1 ||
      !First->getType()->isIntegerTy(ElementBits))
    return false;

  LoadInst *Second = nullptr;
  for (Instruction *I = First->getNextNode(); I; I = I->getNextNode()) {
    if (!I->mayReadOrWriteMemory() && !I->mayHaveSideEffects() &&
        !isa<CallBase>(I) && !isa<FenceInst>(I))
      continue;

    Second = dyn_cast<LoadInst>(I);
    if (!Second || !Second->isSimple() ||
        Second->getPointerAddressSpace() != 1 ||
        !Second->getType()->isIntegerTy(ElementBits))
      return false;
    break;
  }

  if (!Second)
    return false;

  ProgramLoadAddress FirstAddress{First, nullptr, 0};
  ProgramLoadAddress SecondAddress{Second, nullptr, 0};
  if (!decomposeProgramPointer(First->getPointerOperand(), DL,
                               FirstAddress.Base, FirstAddress.Offset) ||
      !decomposeProgramPointer(Second->getPointerOperand(), DL,
                               SecondAddress.Base, SecondAddress.Offset) ||
      FirstAddress.Base != SecondAddress.Base ||
      SecondAddress.Offset !=
          FirstAddress.Offset + static_cast<int64_t>(ElementBits / 8))
    return false;

  Type *WideType = Type::getIntNTy(First->getContext(), ElementBits * 2);
  IRBuilder<> FirstBuilder(First);
  LoadInst *Wide =
      FirstBuilder.CreateAlignedLoad(WideType, First->getPointerOperand(),
                                     Align(1), First->getName() + ".wide");
  Wide->setDebugLoc(First->getDebugLoc());

  Value *Low = FirstBuilder.CreateTrunc(Wide, First->getType(),
                                        First->getName() + ".low");
  cast<Instruction>(Low)->setDebugLoc(First->getDebugLoc());

  IRBuilder<> SecondBuilder(Second);
  Value *Shift =
      SecondBuilder.CreateLShr(Wide, ElementBits, Second->getName() + ".shift");
  cast<Instruction>(Shift)->setDebugLoc(Second->getDebugLoc());
  Value *High = SecondBuilder.CreateTrunc(Shift, Second->getType(),
                                          Second->getName() + ".high");
  cast<Instruction>(High)->setDebugLoc(Second->getDebugLoc());

  First->replaceAllUsesWith(Low);
  Second->replaceAllUsesWith(High);
  Second->eraseFromParent();
  First->eraseFromParent();

  if (ElementBits == 8)
    ++NumLoadsWidenedToI16;
  else
    ++NumLoadsWidenedToI32;
  return true;
}

static unsigned combineProgramLoadPairs(ArrayRef<BasicBlock *> Blocks,
                                        unsigned ElementBits,
                                        const DataLayout &DL) {
  unsigned NumCombined = 0;
  for (BasicBlock *BB : Blocks) {
    SmallVector<LoadInst *, 8> Loads;
    for (Instruction &I : *BB)
      if (auto *Load = dyn_cast<LoadInst>(&I))
        Loads.push_back(Load);

    for (LoadInst *Load : Loads) {
      if (!Load->getParent())
        continue;
      NumCombined += combineProgramLoadPair(Load, ElementBits, DL);
    }
  }
  return NumCombined;
}

static unsigned combineProgramLoadPairs(Function &F, unsigned ElementBits) {
  SmallVector<BasicBlock *, 16> Blocks;
  for (BasicBlock &BB : F)
    Blocks.push_back(&BB);
  return combineProgramLoadPairs(Blocks, ElementBits, F.getDataLayout());
}

class AVMProgramMemoryWidening final : public FunctionPass {
public:
  static char ID;

  AVMProgramMemoryWidening() : FunctionPass(ID) {
    initializeAVMProgramMemoryWideningPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
    if (DisableAVMProgramMemoryWidening ||
        F.hasFnAttribute(Attribute::OptimizeNone) ||
        F.hasFnAttribute(Attribute::MinSize))
      return false;

    AssumptionCache &AC =
        getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);
    DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    OptimizationRemarkEmitter &ORE =
        getAnalysis<OptimizationRemarkEmitterWrapperPass>().getORE();
    ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    TargetTransformInfo &TTI =
        getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);

    SmallVector<Loop *, 8> ByteLoops;
    SmallVector<Loop *, 8> WordLoops;
    SmallPtrSet<Loop *, 8> ByteLoopSet;
    SmallPtrSet<Loop *, 8> WordLoopSet;

    for (Loop *L : LI.getLoopsInPreorder()) {
      if (!L->isInnermost())
        continue;
      if (findSingleProgramStreamLoad(L, 8, SE, DT) &&
          ByteLoopSet.insert(L).second)
        ByteLoops.push_back(L);
      if (findSingleProgramStreamLoad(L, 16, SE, DT) &&
          WordLoopSet.insert(L).second)
        WordLoops.push_back(L);
    }

    bool Changed = false;
    for (Loop *L : ByteLoops) {
      LoadInst *Load = findSingleProgramStreamLoad(L, 8, SE, DT);
      if (!Load)
        continue;

      DebugLoc LoopLoc = L->getStartLoc();
      BasicBlock *Header = L->getHeader();
      Loop *RemainderLoop = nullptr;
      LoopUnrollResult Result =
          unrollByTwo(L, LI, SE, DT, AC, TTI, ORE, &RemainderLoop);
      if (Result == LoopUnrollResult::Unmodified)
        continue;

      Changed = true;
      ++NumByteLoopsUnrolled;
      ORE.emit([&]() {
        return OptimizationRemark(DEBUG_TYPE, "WidenByteProgramLoop", LoopLoc,
                                  Header)
               << "runtime-unrolled an 8-bit program-memory load loop by "
                  "factor two to form a 16-bit main-loop load";
      });

      if (Result == LoopUnrollResult::PartiallyUnrolled) {
        SmallVector<BasicBlock *, 4> Blocks(L->block_begin(), L->block_end());
        if (combineProgramLoadPairs(Blocks, 8, F.getDataLayout()) != 0 &&
            WordLoopSet.insert(L).second)
          WordLoops.push_back(L);
      }
    }

    for (Loop *L : WordLoops) {
      LoadInst *Load = findSingleProgramStreamLoad(L, 16, SE, DT);
      if (!Load)
        continue;

      DebugLoc LoopLoc = L->getStartLoc();
      BasicBlock *Header = L->getHeader();
      Loop *RemainderLoop = nullptr;
      LoopUnrollResult Result =
          unrollByTwo(L, LI, SE, DT, AC, TTI, ORE, &RemainderLoop);
      if (Result == LoopUnrollResult::Unmodified)
        continue;

      Changed = true;
      ++NumWordLoopsUnrolled;
      ORE.emit([&]() {
        return OptimizationRemark(DEBUG_TYPE, "WidenWordProgramLoop", LoopLoc,
                                  Header)
               << "runtime-unrolled a 16-bit program-memory load loop by "
                  "factor two to form a 32-bit main-loop load";
      });

      if (Result == LoopUnrollResult::PartiallyUnrolled) {
        SmallVector<BasicBlock *, 4> Blocks(L->block_begin(), L->block_end());
        combineProgramLoadPairs(Blocks, 16, F.getDataLayout());
      }
    }

    Changed |= combineProgramLoadPairs(F, 8) != 0;
    Changed |= combineProgramLoadPairs(F, 16) != 0;
    return Changed;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<AssumptionCacheTracker>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequiredID(LCSSAID);
    AU.addRequired<OptimizationRemarkEmitterWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<TargetTransformInfoWrapperPass>();
  }
};

} // end anonymous namespace

char AVMProgramMemoryWidening::ID = 0;

INITIALIZE_PASS_BEGIN(AVMProgramMemoryWidening, DEBUG_TYPE, PASS_NAME, false,
                      false)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LCSSAWrapperPass)
INITIALIZE_PASS_DEPENDENCY(OptimizationRemarkEmitterWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(AVMProgramMemoryWidening, DEBUG_TYPE, PASS_NAME, false,
                    false)

FunctionPass *llvm::createAVMProgramMemoryWideningPass() {
  return new AVMProgramMemoryWidening();
}
