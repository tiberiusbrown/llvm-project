//===-- AVMLoopStrengthReduce.cpp - Affine loop addresses ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"

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

struct AddressUse {
  Instruction *Memory;
  const SCEVAddRecExpr *Recurrence;
};

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
  for (Loop *L : LI.getLoopsInPreorder())
    Changed |= reduceLoop(L, LI, SE, F.getDataLayout());

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
