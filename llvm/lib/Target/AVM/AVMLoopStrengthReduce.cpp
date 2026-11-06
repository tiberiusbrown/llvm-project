//===-- AVMLoopStrengthReduce.cpp - Affine loop addresses ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"
#include "llvm/Transforms/Utils/Local.h"

using namespace llvm;

#define DEBUG_TYPE "avm-loop-strength-reduce"

namespace {
class AVMLoopStrengthReduce : public FunctionPass {
public:
  static char ID;
  AVMLoopStrengthReduce() : FunctionPass(ID) {
    initializeAVMLoopStrengthReducePass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
    AU.addPreserved<DominatorTreeWrapperPass>();
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override;
};

class AVMUnitStrideLoops : public FunctionPass {
public:
  static char ID;
  AVMUnitStrideLoops() : FunctionPass(ID) {
    initializeAVMUnitStrideLoopsPass(*PassRegistry::getPassRegistry());
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
    AU.addPreserved<DominatorTreeWrapperPass>();
    AU.setPreservesCFG();
  }
  bool runOnFunction(Function &F) override;
};

struct AddressUse {
  Instruction *Memory;
  const SCEVAddRecExpr *Recurrence;
};

// After global merging, several byte arrays often become constant-offset
// fields of one stream. Use its pointer as the induction variable and exit
// test, eliminating the separate integer index and per-iteration base add.
// Only rewrite exact-trip single-block loops whose old index becomes dead.
static bool reduceUnitStrideLoop(Loop *L, ScalarEvolution &SE,
                                 const DataLayout &DL) {
  BasicBlock *Header = L->getHeader();
  BasicBlock *Preheader = L->getLoopPreheader();
  if (L->getNumBlocks() != 1 || L->getLoopLatch() != Header || !Preheader)
    return false;
  auto *Branch = dyn_cast<CondBrInst>(Header->getTerminator());
  if (!Branch)
    return false;
  auto *Compare = dyn_cast<ICmpInst>(Branch->getCondition());
  if (!Compare || !Compare->isEquality() || !Compare->hasOneUse())
    return false;
  Value *StepValue = Compare->getOperand(0);
  if (isa<ConstantInt>(StepValue))
    StepValue = Compare->getOperand(1);
  auto *Step = dyn_cast<BinaryOperator>(StepValue);
  if (!Step || Step->getOpcode() != Instruction::Add)
    return false;
  auto *Counter = dyn_cast<PHINode>(Step->getOperand(0));
  auto *One = dyn_cast<ConstantInt>(Step->getOperand(1));
  if (!Counter || Counter->getParent() != Header || !One || !One->isOne() ||
      !Counter->getType()->isIntegerTy(16) ||
      Counter->getIncomingValueForBlock(Header) != Step)
    return false;
  unsigned TripCount = SE.getSmallConstantTripCount(L);
  if (TripCount < 2)
    return false;
  unsigned CarriedWords = 0;
  for (PHINode &PN : Header->phis())
    CarriedWords += divideCeil(DL.getTypeSizeInBits(PN.getType()), 16);
  if (CarriedWords > 3)
    return false;

  SmallVector<AddressUse, 4> Uses;
  SmallPtrSet<Instruction *, 16> AddressNodes;
  SmallVector<Instruction *, 8> Worklist;
  DenseMap<Instruction *, unsigned> PointerOperands;
  const SCEVAddRecExpr *Base = nullptr;
  int64_t Stride = 0;
  for (Instruction &I : *Header) {
    Value *Pointer;
    Type *Ty;
    unsigned Operand;
    if (auto *Load = dyn_cast<LoadInst>(&I)) {
      if (Load->isAtomic())
        return false;
      Pointer = Load->getPointerOperand();
      Ty = Load->getType();
      Operand = 0;
    } else if (auto *Store = dyn_cast<StoreInst>(&I)) {
      if (Store->isAtomic())
        return false;
      Pointer = Store->getPointerOperand();
      Ty = Store->getValueOperand()->getType();
      Operand = 1;
    } else {
      if (!I.isTerminator() && I.mayHaveSideEffects())
        return false;
      continue;
    }
    if (Pointer->getType()->getPointerAddressSpace() != 0 ||
        (!Ty->isIntegerTy(8) && !Ty->isIntegerTy(16)))
      return false;
    const auto *AR = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(Pointer));
    if (!AR || AR->getLoop() != L || !AR->isAffine())
      return false;
    if (!Base) {
      Base = AR;
      const auto *Delta = dyn_cast<SCEVConstant>(AR->getStepRecurrence(SE));
      if (!Delta)
        return false;
      Stride = Delta->getAPInt().getSExtValue();
      // The end pointer must not equal an earlier iteration modulo 16 bits.
      if ((Stride != 1 && Stride != 2) || TripCount >= 65536u / Stride)
        return false;
    }
    const auto *Offset = dyn_cast<SCEVConstant>(SE.getMinusSCEV(AR, Base));
    if (!Offset || Offset->getAPInt().getSExtValue() < -32 ||
        Offset->getAPInt().getSExtValue() > 223)
      return false;
    Uses.push_back({&I, AR});
    PointerOperands[&I] = Operand;
    if (auto *Address = dyn_cast<Instruction>(Pointer))
      Worklist.push_back(Address);
  }
  if (Uses.size() < 3)
    return false;
  while (!Worklist.empty()) {
    Instruction *I = Worklist.pop_back_val();
    if (I->getParent() != Header || isa<PHINode>(I) ||
        !AddressNodes.insert(I).second)
      continue;
    for (Value *Operand : I->operand_values())
      if (auto *Def = dyn_cast<Instruction>(Operand))
        Worklist.push_back(Def);
  }
  if (llvm::none_of(Counter->users(), [&](User *U) {
        return AddressNodes.contains(dyn_cast<Instruction>(U));
      }))
    return false;
  for (User *U : Counter->users())
    if (U != Step && !AddressNodes.contains(dyn_cast<Instruction>(U)))
      return false;
  for (User *U : Step->users())
    if (U != Counter && U != Compare)
      return false;
  for (Instruction *I : AddressNodes)
    for (Use &U : I->uses()) {
      auto *User = dyn_cast<Instruction>(U.getUser());
      if (!AddressNodes.contains(User) &&
          (!PointerOperands.contains(User) ||
           U.getOperandNo() != PointerOperands.lookup(User)))
        return false;
    }

  SCEVExpander Expander(SE, "avm.stream", /*PreserveLCSSA=*/true);
  Type *PointerTy = Uses.front().Memory->getOperand(
      PointerOperands.lookup(Uses.front().Memory))->getType();
  Value *Start = Expander.expandCodeFor(Base->getStart(), PointerTy,
                                       Preheader->getTerminator());
  IRBuilder<> PreBuilder(Preheader->getTerminator());
  Value *End = PreBuilder.CreateGEP(PreBuilder.getInt8Ty(), Start,
                                   PreBuilder.getInt16(TripCount * Stride),
                                   "avm.stream.end");
  auto *Pointer = PHINode::Create(PointerTy, 2, "avm.stream.ptr",
                                  Header->getFirstInsertionPt());
  Pointer->addIncoming(Start, Preheader);
  IRBuilder<> B(Branch);
  Value *Next = B.CreateGEP(B.getInt8Ty(), Pointer, B.getInt16(Stride),
                           "avm.stream.next");
  Pointer->addIncoming(Next, Header);
  SmallVector<WeakTrackingVH, 4> DeadAddresses;
  for (const AddressUse &U : Uses) {
    unsigned Operand = PointerOperands.lookup(U.Memory);
    DeadAddresses.push_back(U.Memory->getOperand(Operand));
    const auto *Offset = cast<SCEVConstant>(SE.getMinusSCEV(U.Recurrence, Base));
    IRBuilder<> MemoryBuilder(U.Memory);
    Value *Address = MemoryBuilder.CreateGEP(
        MemoryBuilder.getInt8Ty(), Pointer,
        MemoryBuilder.getInt16(Offset->getAPInt().getSExtValue()), "avm.field");
    U.Memory->setOperand(Operand, Address);
  }
  Value *Condition = Branch->getSuccessor(0) == Header
                         ? B.CreateICmpNE(Next, End)
                         : B.CreateICmpEQ(Next, End);
  Branch->setCondition(Condition);
  SE.forgetLoop(L);
  RecursivelyDeleteTriviallyDeadInstructions(Compare);
  for (WeakTrackingVH &Address : DeadAddresses)
    if (auto *I = dyn_cast_or_null<Instruction>(Address))
      RecursivelyDeleteTriviallyDeadInstructions(I);
  RecursivelyDeleteDeadPHINode(Counter);
  return true;
}

// InstCombine can sink a loop's pointer increments onto its backedge, away
// from the loads using the old pointer. Keep a single-use increment next to
// its load so DAG combining can select AVM's postincrement addressing. This
// only speculates a constant GEP: poison produced on an early-exit path is
// unused, and neither memory access nor any other side effect is moved.
static bool preparePostIncrementAddressing(Loop *L, const DataLayout &DL) {
  BasicBlock *Header = L->getHeader();
  BasicBlock *Latch = L->getLoopLatch();
  if (!Latch)
    return false;
  bool Changed = false;
  for (PHINode &PN : Header->phis()) {
    if (!PN.getType()->isPointerTy() || PN.getNumIncomingValues() != 2)
      continue;
    auto *Increment =
        dyn_cast<GetElementPtrInst>(PN.getIncomingValueForBlock(Latch));
    if (!Increment || Increment->getParent() == Header ||
        Increment->getPointerOperand() != &PN || !Increment->hasOneUse() ||
        !Increment->hasAllConstantIndices())
      continue;

    // The sole address use must be in the header. Moving an increment ahead
    // of other uses of the old pointer could lengthen its live range and
    // require a copy instead of the desired tied load/writeback.
    LoadInst *Load = nullptr;
    for (User *U : PN.users()) {
      if (U == Increment)
        continue;
      auto *Candidate = dyn_cast<LoadInst>(U);
      if (!Candidate || Load || Candidate->getParent() != Header ||
          Candidate->isVolatile() || Candidate->isAtomic()) {
        Load = nullptr;
        break;
      }
      Load = Candidate;
    }
    if (!Load)
      continue;
    unsigned AS = PN.getType()->getPointerAddressSpace();
    // A RAM load with a lower-register base has a slower postincrement
    // encoding than a plain upper-register load plus INC16. If the initial
    // pointer must stay live elsewhere, the extra copy can push the updating
    // pointer out of the upper registers. Limit RAM streams to pointers that
    // can be updated in place; program-space loads have no such distinction.
    // A single static use in a nested loop can still reuse the initial
    // pointer on every parent iteration, keeping it live across the scan.
    if (AS == 0 &&
        (L->getParentLoop() ||
         llvm::any_of(PN.incoming_values(), [&](Value *Incoming) {
           return Incoming != Increment && !Incoming->hasOneUse();
         })))
      continue;
    if (AS == 0) {
      // AS0 also contains the flash window. With a lower-register base its
      // postincrement load can be slower than a displaced load plus INC16.
      // Preserve upper ABI argument pointers that can be updated in place;
      // a pointer computed by an earlier loop has no such placement guarantee.
      Value *Initial = PN.getIncomingValue(0) == Increment
                           ? PN.getIncomingValue(1)
                           : PN.getIncomingValue(0);
      const auto *Arg = dyn_cast<Argument>(Initial);
      if (!Arg || Arg->getArgNo() > 1)
        continue;
      if (Arg->getArgNo() == 1) {
        TypeSize FirstArgBits = DL.getTypeSizeInBits(
            Arg->getParent()->getArg(0)->getType());
        if (FirstArgBits.isScalable() || FirstArgBits.getFixedValue() > 32)
          continue;
      }
    }
    TypeSize StoreSize = DL.getTypeStoreSize(Load->getType());
    if (StoreSize.isScalable())
      continue;
    uint64_t Width = StoreSize.getFixedValue();
    if ((AS == 0 && Width != 1 && Width != 2) ||
        (AS == 1 && (Width < 1 || Width > 4)) || AS > 1)
      continue;
    APInt Offset(DL.getIndexTypeSizeInBits(PN.getType()), 0);
    if (!Increment->accumulateConstantOffset(DL, Offset) || Offset != Width)
      continue;

    Increment->moveAfter(Load);
    Changed = true;
  }
  return Changed;
}

// Select at most one affine data stream and share its pointer across
// constant-offset fields. Generic LSR skips outer loops altogether, and may
// retain a separate base and scaled offset in an innermost loop.
// In particular, indirect accesses in an inner search do not prevent reducing
// the independent affine scan in its parent loop.
static bool reduceLoop(Loop *L, LoopInfo &LI, ScalarEvolution &SE,
                       const DataLayout &DL) {
  if (!L->isLoopSimplifyForm() || !L->getLoopLatch() ||
      SE.getSmallConstantMaxTripCount(L) < 2)
    return false;

  // Reserve half of AVM's eight word registers for invariants, temporaries
  // and inner loops. Do not add another pointer to an already crowded set of
  // loop-carried values. Wide PHIs consume multiple word registers.
  unsigned CarriedWords = 0;
  for (PHINode &PN : L->getHeader()->phis())
    CarriedWords += divideCeil(DL.getTypeSizeInBits(PN.getType()), 16);
  if (CarriedWords > 4)
    return false;

  SmallVector<AddressUse, 8> Uses;
  for (BasicBlock *BB : L->blocks()) {
    if (LI.getLoopFor(BB) != L)
      continue;
    for (Instruction &I : *BB) {
      Value *Pointer;
      Type *Ty;
      if (auto *Load = dyn_cast<LoadInst>(&I)) {
        Pointer = Load->getPointerOperand();
        Ty = Load->getType();
      } else if (auto *Store = dyn_cast<StoreInst>(&I)) {
        Pointer = Store->getPointerOperand();
        Ty = Store->getValueOperand()->getType();
      } else {
        continue;
      }
      // Partial conversion of a wide/mixed stream leaves the old address
      // recurrence live as well as the new one. In particular, 32-bit fields
      // consume register pairs and have no displaced AVM memory form.
      if (Pointer->getType()->getPointerAddressSpace() != 0 ||
          (!Ty->isIntegerTy(8) && !Ty->isIntegerTy(16))) {
        if (!SE.isLoopInvariant(SE.getSCEV(Pointer), L))
          return false;
        continue;
      }
      if (!isa<GetElementPtrInst>(Pointer))
        continue;
      const auto *AR = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(Pointer));
      if (!AR || AR->getLoop() != L || !AR->isAffine())
        continue;
      // An existing pointer IV already does the required strength reduction.
      // Avoid creating another one merely to absorb a field displacement.
      bool HasPointerIV =
          llvm::any_of(L->getHeader()->phis(), [&](PHINode &PN) {
            return PN.getType() == Pointer->getType() &&
                   isa<SCEVConstant>(SE.getMinusSCEV(AR, SE.getSCEV(&PN)));
          });
      if (HasPointerIV)
        continue;
      const auto *Step = dyn_cast<SCEVConstant>(AR->getStepRecurrence(SE));
      if (!Step)
        continue;
      int64_t Stride = Step->getAPInt().getSExtValue();
      // One ADDI replaces a multiply/shift plus a base add. Unit strides
      // already have cheap addressing; large strides need another register.
      if (Stride < -128 || Stride > 127 || (Stride >= -2 && Stride <= 2))
        continue;
      Uses.push_back({&I, AR});
    }
  }
  if (Uses.empty())
    return false;

  auto Displacement = [&](const AddressUse &Use, const SCEV *Base) {
    return dyn_cast<SCEVConstant>(SE.getMinusSCEV(Use.Recurrence, Base));
  };
  const AddressUse *Best = nullptr;
  unsigned BestCount = 0;
  for (const AddressUse &Base : Uses) {
    unsigned Count = 0;
    for (const AddressUse &Use : Uses)
      if (const auto *Offset = Displacement(Use, Base.Recurrence)) {
        int64_t N = Offset->getAPInt().getSExtValue();
        Count += N >= -32 && N <= 223;
      }
    if (Count > BestCount) {
      Best = &Base;
      BestCount = Count;
    }
  }

  SCEVExpander Expander(SE, "avm.iv");
  Expander.disableCanonicalMode();
  auto InsertPt = L->getHeader()->getFirstInsertionPt();
  if (!Expander.isSafeToExpandAt(Best->Recurrence, &*InsertPt))
    return false;
  Value *Pointer = Expander.expandCodeFor(
      Best->Recurrence, Best->Recurrence->getType(), InsertPt);
  for (const AddressUse &Use : Uses) {
    const auto *Offset = Displacement(Use, Best->Recurrence);
    if (!Offset)
      continue;
    int64_t N = Offset->getAPInt().getSExtValue();
    if (N < -32 || N > 223)
      continue;
    IRBuilder<> B(Use.Memory);
    Value *Address =
        B.CreateGEP(B.getInt8Ty(), Pointer, B.getInt16(N), "avm.field");
    unsigned Operand = isa<LoadInst>(Use.Memory) ? 0 : 1;
    Use.Memory->setOperand(Operand, Address);
  }
  return true;
}
} // namespace

bool AVMLoopStrengthReduce::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;
  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  bool Changed = false;
  for (Loop *L : LI.getLoopsInPreorder()) {
    Changed |= reduceLoop(L, LI, SE, F.getDataLayout());
    Changed |= preparePostIncrementAddressing(L, F.getDataLayout());
  }

  // Export proven unsigned-byte ranges to instruction selection. Cross-block
  // word PHIs otherwise become opaque CopyFromReg nodes. Keep the original
  // full-word operands: both MULU8W and MUL16 are then valid, allowing register
  // allocation to use the full register file when upper registers are busy.
  SmallVector<BinaryOperator *, 8> Products;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if (I.getOpcode() == Instruction::Mul && I.getType()->isIntegerTy(16))
        Products.push_back(cast<BinaryOperator>(&I));
  for (BinaryOperator *Product : Products)
    for (unsigned Op = 0; Op != 2; ++Op) {
      auto *V = dyn_cast<Instruction>(Product->getOperand(Op));
      if (!V || SE.getUnsignedRangeMax(SE.getSCEV(V)).ugt(255))
        continue;
      V->setMetadata("avm.unsigned-byte", MDNode::get(F.getContext(), {}));
      Changed = true;
    }
  return Changed;
}

char AVMLoopStrengthReduce::ID = 0;
INITIALIZE_PASS_BEGIN(AVMLoopStrengthReduce, DEBUG_TYPE,
                      "AVM affine loop strength reduction", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_END(AVMLoopStrengthReduce, DEBUG_TYPE,
                    "AVM affine loop strength reduction", false, false)

FunctionPass *llvm::createAVMLoopStrengthReducePass() {
  return new AVMLoopStrengthReduce();
}

bool AVMUnitStrideLoops::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;
  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  bool Changed = false;
  for (Loop *L : LI.getLoopsInPreorder())
    Changed |= reduceUnitStrideLoop(L, SE, F.getDataLayout());
  return Changed;
}

char AVMUnitStrideLoops::ID = 0;
INITIALIZE_PASS_BEGIN(AVMUnitStrideLoops, "avm-unit-stride-loops",
                      "AVM unit-stride pointer loops", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_END(AVMUnitStrideLoops, "avm-unit-stride-loops",
                    "AVM unit-stride pointer loops", false, false)

FunctionPass *llvm::createAVMUnitStrideLoopsPass() {
  return new AVMUnitStrideLoops();
}
