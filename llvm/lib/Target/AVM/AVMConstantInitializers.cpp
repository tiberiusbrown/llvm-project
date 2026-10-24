//===-- AVMConstantInitializers.cpp - Pool constant stores ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "AVMCostModel.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

#define DEBUG_TYPE "avm-constant-initializers"
#define PASS_NAME "AVM constant initialization pooling"

static cl::opt<bool> DisableConstantInitializers(
    "avm-disable-constant-initializers", cl::Hidden, cl::init(false),
    cl::desc("Disable AVM constant-store pooling into bulk memory operations"));

namespace {
struct ConstantStore {
  StoreInst *Store;
  int64_t Offset;
  int64_t Size;
  unsigned Order;
};

static bool hasDenseStorage(Type *Ty, const DataLayout &DL) {
  if (!Ty->isSized() || DL.getTypeStoreSize(Ty).isScalable() ||
      DL.getTypeStoreSize(Ty) != DL.getTypeAllocSize(Ty))
    return false;
  if (auto *I = dyn_cast<IntegerType>(Ty))
    return I->getBitWidth() % 8 == 0;
  if (auto *S = dyn_cast<StructType>(Ty)) {
    if (DL.getStructLayout(S)->hasPadding())
      return false;
    return llvm::all_of(S->elements(), [&](Type *Element) {
      return hasDenseStorage(Element, DL);
    });
  }
  if (auto *A = dyn_cast<ArrayType>(Ty))
    return hasDenseStorage(A->getElementType(), DL);
  if (auto *V = dyn_cast<VectorType>(Ty))
    return hasDenseStorage(V->getElementType(), DL);
  return Ty->isFloatingPointTy() || Ty->isPointerTy();
}

// Do not introduce writes to aggregate padding, undefined bytes, or partial
// scalar storage. Symbolic pointers remain typed constants in the pool, so
// relocations and AVM's three-byte program-pointer representation are retained.
static bool isPoolable(Constant *C, const DataLayout &DL) {
  Type *Ty = C->getType();
  if (!hasDenseStorage(Ty, DL) || isa<UndefValue>(C) || isa<PoisonValue>(C))
    return false;
  if (isa<ConstantInt>(C) || isa<ConstantFP>(C) ||
      isa<ConstantPointerNull>(C) || isa<GlobalValue>(C))
    return true;
  if (isa<ConstantExpr>(C) || Ty->isAggregateType() || Ty->isVectorTy()) {
    if (auto *Data = dyn_cast<ConstantDataSequential>(C)) {
      for (unsigned I = 0; I != Data->getNumElements(); ++I)
        if (!isPoolable(Data->getElementAsConstant(I), DL))
          return false;
      return true;
    }
    for (Value *Operand : C->operands())
      if (!isPoolable(cast<Constant>(Operand), DL))
        return false;
    return true;
  }
  return isa<ConstantAggregateZero>(C);
}

struct MaterializationCost {
  DenseSet<uint16_t> Words;
  SmallPtrSet<Constant *, 16> Symbols;
  uint64_t Cycles = 0, Bytes = 0;

  void add(Constant *C, const DataLayout &DL) {
    if (C->isNullValue()) {
      addWord(0);
      return;
    }
    if (auto *I = dyn_cast<ConstantInt>(C)) {
      addBits(I->getValue());
      return;
    }
    if (auto *F = dyn_cast<ConstantFP>(C)) {
      addBits(F->getValueAPF().bitcastToAPInt());
      return;
    }
    if (auto *Data = dyn_cast<ConstantDataSequential>(C)) {
      for (unsigned I = 0; I != Data->getNumElements(); ++I)
        add(Data->getElementAsConstant(I), DL);
      return;
    }
    if (C->getType()->isAggregateType() || C->getType()->isVectorTy()) {
      for (Value *Operand : C->operands())
        add(cast<Constant>(Operand), DL);
      return;
    }
    if (Symbols.insert(C).second) {
      unsigned Parts = divideCeil(
          DL.getTypeStoreSize(C->getType()).getFixedValue(), uint64_t(2));
      Cycles += Parts * AVM::getFixedCycles(AVM::AVMCostKind::Ldi16Upper);
      Bytes += Parts * 3;
    }
  }

  void addWord(uint16_t Word) {
    if (!Words.insert(Word).second)
      return;
    using AVM::AVMCostKind;
    Cycles += AVM::getFixedCycles(!Word        ? AVMCostKind::XorUpper
                                  : Word < 256 ? AVMCostKind::Ldi8Upper
                                               : AVMCostKind::Ldi16Upper);
    Bytes += !Word ? 1 : Word < 256 ? 2 : 3;
  }

  void addBits(const APInt &Bits) {
    for (unsigned I = 0; I < Bits.getBitWidth(); I += 16)
      addWord(Bits.extractBitsAsZExtValue(std::min(16u, Bits.getBitWidth() - I),
                                          I));
  }
};

class AVMConstantInitializers final : public ModulePass {
  DenseMap<Constant *, GlobalVariable *> Pools;

  bool emitCopy(MutableArrayRef<ConstantStore> Stores, Module &M) {
    const DataLayout &DL = M.getDataLayout();
    uint64_t Size =
        Stores.back().Offset + Stores.back().Size - Stores.front().Offset;
    if (Size < 16 || Size > UINT16_MAX)
      return false;
    MaterializationCost Cost;
    SmallVector<Constant *, 32> Values;
    ConstantInt *Fill = nullptr;
    bool Uniform = true;
    ConstantStore *Last = &Stores.front();
    for (ConstantStore &S : Stores) {
      auto *Value = cast<Constant>(S.Store->getValueOperand());
      Values.push_back(Value);
      Cost.add(Value, DL);
      auto *Byte = dyn_cast_or_null<ConstantInt>(isBytewiseValue(Value, DL));
      if (!Byte || (Fill && Fill != Byte))
        Uniform = false;
      Fill = Byte;
      if (S.Order > Last->Order)
        Last = &S;
    }
    // Measured interpreter costs: memcpy_P(n) = 233 + 17*n for n > 0.
    // Reserve another 250 cycles for argument setup and register preservation.
    // Count each distinct immediate only once, allowing inline register reuse.
    // The postincrement word-store cost includes pointer advancement.
    uint64_t InlineCycles =
        (Size / 2) * AVM::getFixedCycles(AVM::AVMCostKind::St16PostIncDense) +
        (Size % 2) * AVM::getFixedCycles(AVM::AVMCostKind::St8Upper) +
        Cost.Cycles;
    uint64_t InlineBytes = divideCeil(Size, 2) * 2 + Cost.Bytes;
    uint64_t BulkCycles = Uniform ? 300 + 6 * Size : 483 + 17 * Size;
    uint64_t BulkBytes = Uniform ? 10 : 14 + Size;
    if (BulkCycles >= InlineCycles || BulkBytes >= InlineBytes)
      return false;

    IRBuilder<> B(Last->Store);
    B.SetCurrentDebugLocation(Last->Store->getDebugLoc());
    Value *Dst = Stores.front().Store->getPointerOperand();
    if (Uniform) {
      B.CreateMemSet(Dst, Fill, B.getInt16(Size),
                     Stores.front().Store->getAlign());
    } else {
      Constant *Data = ConstantStruct::getAnon(M.getContext(), Values, true);
      GlobalVariable *&Pool = Pools[Data];
      if (!Pool) {
        Pool = new GlobalVariable(
            M, Data->getType(), true, GlobalValue::PrivateLinkage, Data,
            ".avm.init", nullptr, GlobalValue::NotThreadLocal, 1);
        Pool->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
        Pool->setAlignment(Align(1));
        Pool->setMetadata("avm.constant.initializer",
                          MDNode::get(M.getContext(), {}));
      }
      B.CreateMemCpy(Dst, Stores.front().Store->getAlign(), Pool, Align(1),
                     B.getInt16(Size));
    }
    for (ConstantStore &S : Stores)
      S.Store->eraseFromParent();
    return true;
  }

public:
  static char ID;
  AVMConstantInitializers() : ModulePass(ID) {}
  StringRef getPassName() const override { return PASS_NAME; }

  bool runOnModule(Module &M) override {
    if (DisableConstantInitializers || skipModule(M))
      return false;
    Pools.clear();
    const DataLayout &DL = M.getDataLayout();
    bool Changed = false;
    for (Function &F : M) {
      if (F.hasOptNone())
        continue;
      for (BasicBlock &BB : F) {
        SmallVector<ConstantStore, 32> Stores;
        Value *Base = nullptr;
        auto Flush = [&]() {
          llvm::sort(Stores, [](const auto &A, const auto &B) {
            return A.Offset < B.Offset;
          });
          for (unsigned Begin = 0, End; Begin < Stores.size(); Begin = End) {
            End = Begin + 1;
            while (End < Stores.size() &&
                   Stores[End].Offset ==
                       Stores[End - 1].Offset + Stores[End - 1].Size &&
                   Stores[End].Offset + Stores[End].Size -
                           Stores[Begin].Offset <=
                       UINT16_MAX)
              ++End;
            Changed |=
                emitCopy(MutableArrayRef(Stores).slice(Begin, End - Begin), M);
          }
          Stores.clear();
          Base = nullptr;
        };
        for (auto It = BB.begin(); It != BB.end();) {
          Instruction &I = *It++;
          auto *S = dyn_cast<StoreInst>(&I);
          auto *C = S ? dyn_cast<Constant>(S->getValueOperand()) : nullptr;
          if (S && S->isSimple() && S->getPointerAddressSpace() == 0 && C &&
              isPoolable(C, DL)) {
            int64_t Offset;
            Value *NewBase = GetPointerBaseWithConstantOffset(
                S->getPointerOperand(), Offset, DL);
            int64_t Size = DL.getTypeStoreSize(C->getType()).getFixedValue();
            if (!Size || Size > UINT16_MAX) {
              Flush();
              continue;
            }
            bool Overlaps =
                llvm::any_of(Stores, [&](const ConstantStore &Other) {
                  return Offset < Other.Offset + Other.Size &&
                         Other.Offset < Offset + Size;
                });
            if (NewBase != Base || Stores.size() == 128 || Overlaps)
              Flush();
            Base = NewBase;
            Stores.push_back({S, Offset, Size, unsigned(Stores.size())});
          } else if (I.mayReadOrWriteMemory() || I.mayHaveSideEffects() ||
                     isa<CallBase>(I) || I.isTerminator()) {
            Flush();
          }
        }
        Flush();
      }
    }
    return Changed;
  }
};
} // namespace

char AVMConstantInitializers::ID = 0;
INITIALIZE_PASS(AVMConstantInitializers, DEBUG_TYPE, PASS_NAME, false, false)
ModulePass *llvm::createAVMConstantInitializersPass() {
  return new AVMConstantInitializers();
}
