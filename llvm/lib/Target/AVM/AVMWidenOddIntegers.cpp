//===-- AVMWidenOddIntegers.cpp - Native-container integer operations ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AVM registers are 16 bits wide and register pairs hold 32 bits. Integers of
// other widths, such as the i9 or i40 types of C23 _BitInt(N), are promoted or
// expanded by SelectionDAG legalization. That path cannot select some shifts,
// bit counts, and conversions on those widths, and it mishandles the sign
// extension of some values. This pass computes such operations in the next
// native container instead: i16 up to 16 bits, i32 up to 32 bits, and i64
// otherwise. Operands are widened with zero or sign extension, and the result
// is truncated back to the original width.
//
// The rewritten operations are shifts, bit counts, signed division and
// remainder, signed comparisons, sign extensions, overflow intrinsics, and
// integer/floating conversions on odd widths. Each result equals the original
// modulo 2^N. Shift amounts at or above the width remain poison, as they are
// in LLVM IR.
//
// Sign extension from a source wider than 16 bits is written as
// (zext(x) ^ sign) - sign. The DAG combiner drops the sign extension of some
// values derived from 24-bit memory pieces, so this form keeps the extension.
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "avm-widen-odd-integers"
#define PASS_NAME "AVM odd integer widening"

namespace {

// Widths that instruction selection handles directly, plus i1 for booleans.
// Everything else between 2 and 63 bits is widened.
static bool needsWidening(Type *Ty) {
  auto *IT = dyn_cast<IntegerType>(Ty);
  if (!IT)
    return false;
  unsigned Bits = IT->getBitWidth();
  return Bits > 1 && Bits < 64 && Bits != 8 && Bits != 16 && Bits != 32;
}

static IntegerType *getContainerType(LLVMContext &Ctx, unsigned Bits) {
  return IntegerType::get(Ctx, Bits <= 16 ? 16 : Bits <= 32 ? 32 : 64);
}

// Sign-extends V to To. Sources wider than 16 bits use (zext(V) ^ sign) - sign.
// The DAG combiner drops sign extensions of some values that come from 24-bit
// memory pieces (sext_inreg of i24), so compares and extensions of those widths
// must not use sext. Narrow sources keep the native extension.
static Value *signExtendOdd(IRBuilder<> &B, Value *V, IntegerType *To) {
  unsigned FromBits = V->getType()->getIntegerBitWidth();
  if (FromBits <= 16)
    return B.CreateSExt(V, To);
  Value *Extended = B.CreateZExt(V, To);
  Constant *Sign = ConstantInt::get(
      To, APInt::getOneBitSet(To->getBitWidth(), FromBits - 1));
  return B.CreateSub(B.CreateXor(Extended, Sign), Sign);
}

static bool isOverflowIntrinsic(Intrinsic::ID ID) {
  switch (ID) {
  case Intrinsic::sadd_with_overflow:
  case Intrinsic::uadd_with_overflow:
  case Intrinsic::ssub_with_overflow:
  case Intrinsic::usub_with_overflow:
  case Intrinsic::smul_with_overflow:
  case Intrinsic::umul_with_overflow:
    return true;
  default:
    return false;
  }
}

static bool isSignedOverflow(Intrinsic::ID ID) {
  return ID == Intrinsic::sadd_with_overflow ||
         ID == Intrinsic::ssub_with_overflow ||
         ID == Intrinsic::smul_with_overflow;
}

static bool isWidenedOperation(Instruction &I) {
  if (auto *CI = dyn_cast<CastInst>(&I)) {
    switch (CI->getOpcode()) {
    case Instruction::SIToFP:
    case Instruction::UIToFP:
      return needsWidening(CI->getOperand(0)->getType());
    case Instruction::FPToSI:
    case Instruction::FPToUI:
      return needsWidening(CI->getType());
    default:
      break;
    }
  }
  if (auto *IC = dyn_cast<ICmpInst>(&I))
    return IC->isSigned() && needsWidening(IC->getOperand(0)->getType()) &&
           IC->getOperand(0)->getType()->getIntegerBitWidth() > 16;
  if (auto *SE = dyn_cast<SExtInst>(&I))
    return needsWidening(SE->getOperand(0)->getType()) &&
           SE->getOperand(0)->getType()->getIntegerBitWidth() > 16;
  if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
    switch (BO->getOpcode()) {
    case Instruction::Shl:
    case Instruction::LShr:
    case Instruction::AShr:
    case Instruction::SDiv:
    case Instruction::SRem:
      return needsWidening(BO->getType());
    default:
      return false;
    }
  }
  if (auto *II = dyn_cast<IntrinsicInst>(&I)) {
    if (isOverflowIntrinsic(II->getIntrinsicID()))
      return needsWidening(cast<StructType>(II->getType())->getElementType(0));
    switch (II->getIntrinsicID()) {
    case Intrinsic::ctpop:
    case Intrinsic::ctlz:
    case Intrinsic::cttz:
      return needsWidening(II->getType());
    default:
      return false;
    }
  }
  return false;
}

// The low N bits of a shift depend only on the low N bits of its left operand
// for shl, and on the zero or sign extension for the right shifts. Shift
// amounts are zero-extended; amounts of at least N are poison in the source.
static void widenShift(BinaryOperator &I) {
  IRBuilder<> B(&I);
  unsigned Bits = I.getType()->getIntegerBitWidth();
  IntegerType *CTy = getContainerType(I.getContext(), Bits);
  bool IsArithmetic = I.getOpcode() == Instruction::AShr;
  Value *LHS = IsArithmetic ? signExtendOdd(B, I.getOperand(0), CTy)
                            : B.CreateZExt(I.getOperand(0), CTy);
  Value *RHS = B.CreateZExt(I.getOperand(1), CTy);
  Value *Shifted = B.CreateBinOp(I.getOpcode(), LHS, RHS);
  Value *Result = B.CreateTrunc(Shifted, I.getType());
  I.replaceAllUsesWith(Result);
  I.eraseFromParent();
}

// Compares widen both operands. Signed predicates need the sign extension;
// unsigned predicates and equality need only zero extension.
static void widenCompare(ICmpInst &I) {
  IRBuilder<> B(&I);
  unsigned Bits = I.getOperand(0)->getType()->getIntegerBitWidth();
  IntegerType *CTy = getContainerType(I.getContext(), Bits);
  bool IsSigned = I.isSigned();
  Value *LHS = IsSigned ? signExtendOdd(B, I.getOperand(0), CTy)
                        : B.CreateZExt(I.getOperand(0), CTy);
  Value *RHS = IsSigned ? signExtendOdd(B, I.getOperand(1), CTy)
                        : B.CreateZExt(I.getOperand(1), CTy);
  Value *Result = B.CreateICmp(I.getPredicate(), LHS, RHS);
  I.replaceAllUsesWith(Result);
  I.eraseFromParent();
}

// Sign extension from an odd wide source. The extension is computed in the
// container of the destination, and truncated when the destination is odd.
static void widenSExt(SExtInst &I) {
  IRBuilder<> B(&I);
  LLVMContext &Ctx = I.getContext();
  unsigned DestBits = I.getType()->getIntegerBitWidth();
  IntegerType *CTy = needsWidening(I.getType())
                         ? getContainerType(Ctx, DestBits)
                         : cast<IntegerType>(I.getType());
  Value *Result = signExtendOdd(B, I.getOperand(0), CTy);
  if (CTy != I.getType())
    Result = B.CreateTrunc(Result, I.getType());
  I.replaceAllUsesWith(Result);
  I.eraseFromParent();
}

// Overflow intrinsics on odd widths. The container operation reports overflow
// of the wide result, and the narrow result is overflowed when it does not
// survive truncation to the original width.
static void widenOverflow(IntrinsicInst &I) {
  IRBuilder<> B(&I);
  LLVMContext &Ctx = I.getContext();
  Intrinsic::ID ID = I.getIntrinsicID();
  Type *ValTy = cast<StructType>(I.getType())->getElementType(0);
  IntegerType *CTy = getContainerType(Ctx, ValTy->getIntegerBitWidth());
  bool IsSigned = isSignedOverflow(ID);
  Value *LHS = IsSigned ? signExtendOdd(B, I.getArgOperand(0), CTy)
                        : B.CreateZExt(I.getArgOperand(0), CTy);
  Value *RHS = IsSigned ? signExtendOdd(B, I.getArgOperand(1), CTy)
                        : B.CreateZExt(I.getArgOperand(1), CTy);
  Value *Wide = B.CreateIntrinsic(ID, {CTy}, {LHS, RHS});
  Value *Sum = B.CreateExtractValue(Wide, 0);
  Value *WideOverflow = B.CreateExtractValue(Wide, 1);
  Value *Narrow = B.CreateTrunc(Sum, ValTy);
  Value *Back =
      IsSigned ? signExtendOdd(B, Narrow, CTy) : B.CreateZExt(Narrow, CTy);
  Value *Lost = B.CreateICmpNE(Back, Sum);
  Value *Result = PoisonValue::get(I.getType());
  Result = B.CreateInsertValue(Result, Narrow, 0);
  Result = B.CreateInsertValue(Result, B.CreateOr(WideOverflow, Lost), 1);
  I.replaceAllUsesWith(Result);
  I.eraseFromParent();
}

// Signed division and remainder take sign-extended operands. The quotient is
// truncated; the division itself cannot overflow the container.
static void widenSignedDivRem(BinaryOperator &I) {
  IRBuilder<> B(&I);
  unsigned Bits = I.getType()->getIntegerBitWidth();
  IntegerType *CTy = getContainerType(I.getContext(), Bits);
  Value *LHS = signExtendOdd(B, I.getOperand(0), CTy);
  Value *RHS = signExtendOdd(B, I.getOperand(1), CTy);
  Value *Result =
      B.CreateTrunc(B.CreateBinOp(I.getOpcode(), LHS, RHS), I.getType());
  I.replaceAllUsesWith(Result);
  I.eraseFromParent();
}

// Integer to floating conversion of an odd-width source, in the container.
static void widenIntToFP(CastInst &I) {
  IRBuilder<> B(&I);
  unsigned Bits = I.getOperand(0)->getType()->getIntegerBitWidth();
  IntegerType *CTy = getContainerType(I.getContext(), Bits);
  bool IsSigned = I.getOpcode() == Instruction::SIToFP;
  Value *Src = IsSigned ? signExtendOdd(B, I.getOperand(0), CTy)
                        : B.CreateZExt(I.getOperand(0), CTy);
  Value *Result = IsSigned ? B.CreateSIToFP(Src, I.getType())
                           : B.CreateUIToFP(Src, I.getType());
  I.replaceAllUsesWith(Result);
  I.eraseFromParent();
}

// Floating to integer conversion of an odd-width destination, in the container.
static void widenFPToInt(CastInst &I) {
  IRBuilder<> B(&I);
  unsigned Bits = I.getType()->getIntegerBitWidth();
  IntegerType *CTy = getContainerType(I.getContext(), Bits);
  bool IsSigned = I.getOpcode() == Instruction::FPToSI;
  Value *Wide = IsSigned ? B.CreateFPToSI(I.getOperand(0), CTy)
                         : B.CreateFPToUI(I.getOperand(0), CTy);
  Value *Result = B.CreateTrunc(Wide, I.getType());
  I.replaceAllUsesWith(Result);
  I.eraseFromParent();
}

// Zero extension leaves the population count unchanged.
static void widenCtpop(IntrinsicInst &I) {
  IRBuilder<> B(&I);
  unsigned Bits = I.getType()->getIntegerBitWidth();
  IntegerType *CTy = getContainerType(I.getContext(), Bits);
  Value *Extended = B.CreateZExt(I.getArgOperand(0), CTy);
  Value *Count = B.CreateUnaryIntrinsic(Intrinsic::ctpop, Extended);
  Value *Result = B.CreateTrunc(Count, I.getType());
  I.replaceAllUsesWith(Result);
  I.eraseFromParent();
}

// Zero extension adds exactly (container - N) leading zeros, so subtracting
// that difference restores the count. A zero input counts the whole container,
// and the subtraction maps that count to N.
static void widenCtlz(IntrinsicInst &I) {
  IRBuilder<> B(&I);
  unsigned Bits = I.getType()->getIntegerBitWidth();
  IntegerType *CTy = getContainerType(I.getContext(), Bits);
  unsigned Extra = CTy->getBitWidth() - Bits;
  Value *Extended = B.CreateZExt(I.getArgOperand(0), CTy);
  Value *Count =
      B.CreateIntrinsic(Intrinsic::ctlz, {CTy}, {Extended, I.getArgOperand(1)});
  Value *Adjusted = B.CreateSub(Count, ConstantInt::get(CTy, Extra));
  Value *Result = B.CreateTrunc(Adjusted, I.getType());
  I.replaceAllUsesWith(Result);
  I.eraseFromParent();
}

// Zero extension places zeros above bit N, so a zero input counts the whole
// container. Clamping to N gives the original result for every input.
static void widenCttz(IntrinsicInst &I) {
  IRBuilder<> B(&I);
  unsigned Bits = I.getType()->getIntegerBitWidth();
  IntegerType *CTy = getContainerType(I.getContext(), Bits);
  Value *Extended = B.CreateZExt(I.getArgOperand(0), CTy);
  Value *Count =
      B.CreateIntrinsic(Intrinsic::cttz, {CTy}, {Extended, I.getArgOperand(1)});
  Value *Clamped = B.CreateBinaryIntrinsic(Intrinsic::umin, Count,
                                           ConstantInt::get(CTy, Bits));
  Value *Result = B.CreateTrunc(Clamped, I.getType());
  I.replaceAllUsesWith(Result);
  I.eraseFromParent();
}

class AVMWidenOddIntegers final : public FunctionPass {
public:
  static char ID;

  AVMWidenOddIntegers() : FunctionPass(ID) {
    initializeAVMWidenOddIntegersPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
    SmallVector<Instruction *, 8> Work;
    for (Instruction &I : instructions(F))
      if (isWidenedOperation(I))
        Work.push_back(&I);

    for (Instruction *I : Work) {
      if (auto *IC = dyn_cast<ICmpInst>(I)) {
        widenCompare(*IC);
        continue;
      }
      if (auto *SE = dyn_cast<SExtInst>(I)) {
        widenSExt(*SE);
        continue;
      }
      if (auto *CI = dyn_cast<CastInst>(I)) {
        if (CI->getOpcode() == Instruction::SIToFP ||
            CI->getOpcode() == Instruction::UIToFP)
          widenIntToFP(*CI);
        else
          widenFPToInt(*CI);
        continue;
      }
      if (auto *BO = dyn_cast<BinaryOperator>(I)) {
        if (BO->getOpcode() == Instruction::SDiv ||
            BO->getOpcode() == Instruction::SRem)
          widenSignedDivRem(*BO);
        else
          widenShift(*BO);
        continue;
      }
      auto *II = cast<IntrinsicInst>(I);
      if (isOverflowIntrinsic(II->getIntrinsicID())) {
        widenOverflow(*II);
        continue;
      }
      switch (II->getIntrinsicID()) {
      case Intrinsic::ctpop:
        widenCtpop(*II);
        break;
      case Intrinsic::ctlz:
        widenCtlz(*II);
        break;
      case Intrinsic::cttz:
        widenCttz(*II);
        break;
      default:
        llvm_unreachable("unexpected intrinsic in widening worklist");
      }
    }
    return !Work.empty();
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }
};

} // namespace

char AVMWidenOddIntegers::ID = 0;

INITIALIZE_PASS(AVMWidenOddIntegers, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMWidenOddIntegersPass() {
  return new AVMWidenOddIntegers();
}
