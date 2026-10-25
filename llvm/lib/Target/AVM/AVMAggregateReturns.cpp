//===-- AVMAggregateReturns.cpp - Aggregate update ABI -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Coalesce a returned byval object with its caller's overwritten source. This
// is an internal ABI specialization, before SelectionDAG creates the byval
// copy: the callee receives one ordinary read/write pointer instead of separate
// byval and sret pointers. Neither the public ABI nor escaping objects change.
//
//===----------------------------------------------------------------------===//

#include "AVM.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

#define DEBUG_TYPE "avm-aggregate-returns"
#define PASS_NAME "AVM internal aggregate return specialization"

static cl::opt<bool> DisableAggregateReturns(
    "avm-disable-aggregate-returns", cl::Hidden, cl::init(false),
    cl::desc("Disable AVM internal aggregate update ABI specialization"));

namespace {
static bool isFullCopy(const MemCpyInst &Copy, Value *Dst, Value *Src,
                       uint64_t Size) {
  auto *Length = dyn_cast<ConstantInt>(Copy.getLength());
  return !Copy.isVolatile() && Copy.getRawDest() == Dst &&
         Copy.getRawSource() == Src && Length && Length->equalsInt(Size);
}

static bool fits(uint64_t Offset, TypeSize Size, uint64_t ObjectSize) {
  return !Size.isScalable() && Offset <= ObjectSize &&
         Size.getFixedValue() <= ObjectSize - Offset;
}

// Prove that an object's address is used only for bounded, ordinary memory
// accesses. In the caller, byval calls are also safe: they expose a copy, not
// this address. In the callee, even a nocapture call or a pointer comparison
// could observe the former parameter object's identity, so reject them.
static bool hasPrivateAddress(Value *Object, uint64_t Size,
                              const DataLayout &DL, bool IsCaller) {
  SmallVector<std::pair<Value *, uint64_t>, 16> Worklist{{Object, 0}};
  SmallPtrSet<Value *, 32> Visited;
  while (!Worklist.empty()) {
    auto [Ptr, Offset] = Worklist.pop_back_val();
    if (!Visited.insert(Ptr).second)
      continue;
    for (Use &U : Ptr->uses()) {
      User *User = U.getUser();
      if (auto *GEP = dyn_cast<GetElementPtrInst>(User)) {
        APInt Delta(DL.getIndexTypeSizeInBits(Ptr->getType()), 0);
        if (!GEP->accumulateConstantOffset(DL, Delta) || Delta.isNegative() ||
            Delta.getLimitedValue() > Size - Offset)
          return false;
        Worklist.emplace_back(GEP, Offset + Delta.getZExtValue());
        continue;
      }
      if (auto *Cast = dyn_cast<BitCastInst>(User)) {
        Worklist.emplace_back(Cast, Offset);
        continue;
      }
      if (auto *Load = dyn_cast<LoadInst>(User)) {
        if (!Load->isSimple() ||
            !fits(Offset, DL.getTypeStoreSize(Load->getType()), Size))
          return false;
        continue;
      }
      if (auto *Store = dyn_cast<StoreInst>(User)) {
        if (&U != &Store->getOperandUse(StoreInst::getPointerOperandIndex()) ||
            !Store->isSimple() ||
            !fits(Offset,
                  DL.getTypeStoreSize(Store->getValueOperand()->getType()),
                  Size))
          return false;
        continue;
      }
      if (auto *Mem = dyn_cast<MemIntrinsic>(User)) {
        auto *Length = dyn_cast<ConstantInt>(Mem->getLength());
        if (Mem->isVolatile() || !Length ||
            Length->getValue().ugt(Size - Offset))
          return false;
        continue;
      }
      if (IsCaller) {
        if (isa<LifetimeIntrinsic>(User))
          continue;
        if (auto *Call = dyn_cast<CallBase>(User)) {
          if (!Call->isArgOperand(&U))
            return false;
          unsigned ArgNo = Call->getArgOperandNo(&U);
          Type *ByVal = Call->getParamByValType(ArgNo);
          if (ByVal && fits(Offset, DL.getTypeAllocSize(ByVal), Size))
            continue;
        }
      }
      return false;
    }
  }
  return true;
}

static bool isObject(AllocaInst *AI, uint64_t Size, const DataLayout &DL) {
  if (!AI || !AI->isStaticAlloca() || AI->getAddressSpace() != 0)
    return false;
  std::optional<TypeSize> AllocSize = AI->getAllocationSize(DL);
  return AllocSize && !AllocSize->isScalable() &&
         AllocSize->getFixedValue() == Size;
}

struct UpdateCall {
  CallInst *Call;
  MemCpyInst *CopyBack;
  AllocaInst *Result;
};

// The result must be a temporary used only by this call and its copy back.
// The source's private address ensures that moving modifications into the
// call cannot be observed through another argument, global, or callback.
static std::optional<UpdateCall> getUpdateCall(CallInst &Call, unsigned RetNo,
                                               unsigned InputNo, uint64_t Size,
                                               const DataLayout &DL) {
  if (Call.isMustTailCall() || Call.hasOperandBundles())
    return std::nullopt;
  auto *Result = dyn_cast<AllocaInst>(Call.getArgOperand(RetNo));
  auto *Source = dyn_cast<AllocaInst>(Call.getArgOperand(InputNo));
  auto *Copy = dyn_cast_or_null<MemCpyInst>(Call.getNextNode());
  if (!isObject(Result, Size, DL) || !isObject(Source, Size, DL) ||
      Result == Source || !Copy || !isFullCopy(*Copy, Source, Result, Size) ||
      !hasPrivateAddress(Source, Size, DL, /*IsCaller=*/true))
    return std::nullopt;
  for (Use &U : Result->uses()) {
    if (U.getUser() == &Call) {
      if (&U != &Call.getArgOperandUse(RetNo))
        return std::nullopt;
    } else if (U.getUser() != Copy) {
      if (!isa<LifetimeIntrinsic>(U.getUser()))
        return std::nullopt;
    }
  }
  return UpdateCall{&Call, Copy, Result};
}

static bool specialize(Function &F) {
  // Non-local exits would expose the source before the C assignment commits.
  // Exclude them, address-taken functions, and optnone functions explicitly.
  if (!F.hasLocalLinkage() || F.isDeclaration() || F.isVarArg() ||
      !F.getReturnType()->isVoidTy() || !F.doesNotThrow() ||
      !F.hasFnAttribute(Attribute::WillReturn) ||
      F.hasFnAttribute(Attribute::ReturnsTwice) ||
      F.hasFnAttribute(Attribute::OptimizeNone) || F.hasAddressTaken() ||
      F.hasMetadata(LLVMContext::MD_callback) || F.use_empty())
    return false;
  const DataLayout &DL = F.getDataLayout();
  Argument *Ret = nullptr;
  Argument *Input = nullptr;
  SmallVector<MemCpyInst *, 4> ReturnCopies;
  // IPO (notably LTO argument promotion) can remove sret from internal
  // functions. Infer the result/input relationship from all returning paths
  // instead of relying on the original ABI marker surviving optimization.
  for (BasicBlock &BB : F) {
    auto *Return = dyn_cast<ReturnInst>(BB.getTerminator());
    if (!Return)
      continue;
    auto *Copy = dyn_cast_or_null<MemCpyInst>(Return->getPrevNode());
    auto *Dst = Copy ? dyn_cast<Argument>(Copy->getRawDest()) : nullptr;
    auto *Src = Copy ? dyn_cast<Argument>(Copy->getRawSource()) : nullptr;
    if (!Dst || !Src || Dst == Src || Dst->getParent() != &F ||
        Src->getParent() != &F || Dst->hasByValAttr() || !Src->hasByValAttr() ||
        (Ret && Ret != Dst) || (Input && Input != Src))
      return false;
    Ret = Dst;
    Input = Src;
    ReturnCopies.push_back(Copy);
  }
  if (!Input || Input->getType() != Ret->getType() ||
      Ret->getType()->getPointerAddressSpace() != 0 ||
      Ret->getNumUses() != ReturnCopies.size())
    return false;
  Type *Ty = Input->getParamByValType();
  TypeSize AllocSize = DL.getTypeAllocSize(Ty);
  if (AllocSize.isScalable() || AllocSize.getFixedValue() <= 4 ||
      (Ret->hasStructRetAttr() && Ret->getParamStructRetType() != Ty))
    return false;
  uint64_t Size = AllocSize.getFixedValue();
  if (!all_of(ReturnCopies,
              [&](MemCpyInst *Copy) {
                return isFullCopy(*Copy, Ret, Input, Size);
              }) ||
      !hasPrivateAddress(Input, Size, DL, /*IsCaller=*/false))
    return false;

  SmallVector<UpdateCall, 4> Calls;
  for (User *U : F.users()) {
    auto *Call = dyn_cast<CallInst>(U);
    if (!Call || Call->getCalledOperand() != &F ||
        Call->getCallingConv() != F.getCallingConv() ||
        Call->getParamByValType(Input->getArgNo()) != Ty ||
        Call->getParamStructRetType(Ret->getArgNo()) !=
            Ret->getParamStructRetType() ||
        Call->paramHasAttr(Ret->getArgNo(), Attribute::ByVal))
      return false;
    auto Update =
        getUpdateCall(*Call, Ret->getArgNo(), Input->getArgNo(), Size, DL);
    if (!Update ||
        cast<AllocaInst>(Call->getArgOperand(Input->getArgNo()))->getAlign() <
            Input->getParamAlign().valueOrOne())
      return false;
    Calls.push_back(*Update);
  }

  // All callers agree on the specialization, so there is no wrapper or cloned
  // fallback body to increase code size. Preserve other arguments and their
  // ABI.
  LLVMContext &Ctx = F.getContext();
  SmallVector<Type *, 8> ParamTypes;
  SmallVector<AttributeSet, 8> ParamAttrs;
  AttributeSet ObjectAttrs = AttributeSet::get(
      Ctx,
      {Attribute::get(Ctx, Attribute::NonNull),
       Attribute::getWithAlignment(Ctx, Input->getParamAlign().valueOrOne()),
       Attribute::getWithDereferenceableBytes(Ctx, Size)});
  for (Argument &Arg : F.args()) {
    if (&Arg == Ret)
      continue;
    ParamTypes.push_back(Arg.getType());
    ParamAttrs.push_back(&Arg == Input
                             ? ObjectAttrs
                             : F.getAttributes().getParamAttrs(Arg.getArgNo()));
  }
  auto *FTy = FunctionType::get(F.getReturnType(), ParamTypes, false);
  auto *NewF = Function::Create(FTy, F.getLinkage(), F.getAddressSpace(),
                                F.getName() + ".inplace", F.getParent());
  ValueToValueMapTy Map;
  auto NewArg = NewF->arg_begin();
  for (Argument &Arg : F.args()) {
    if (&Arg == Ret)
      continue;
    NewArg->setName(Arg.getName());
    Map[&Arg] = &*NewArg++;
  }
  Map[Ret] = Map[Input];
  SmallVector<ReturnInst *, 4> Returns;
  CloneFunctionInto(NewF, &F, Map, CloneFunctionChangeType::LocalChangesOnly,
                    Returns);
  NewF->setAttributes(AttributeList::get(Ctx, F.getAttributes().getFnAttrs(),
                                         F.getAttributes().getRetAttrs(),
                                         ParamAttrs));
  // A byval object's writes were private. They now modify argument memory.
  NewF->setMemoryEffects(F.getMemoryEffects() | MemoryEffects::argMemOnly());
  for (MemCpyInst *Copy : ReturnCopies)
    cast<Instruction>(Map[Copy])->eraseFromParent();

  for (UpdateCall &Update : Calls) {
    CallInst &Old = *Update.Call;
    SmallVector<Value *, 8> Args;
    SmallVector<AttributeSet, 8> Attrs;
    for (unsigned I = 0; I != Old.arg_size(); ++I) {
      if (I == Ret->getArgNo())
        continue;
      Args.push_back(Old.getArgOperand(I));
      Attrs.push_back(I == Input->getArgNo()
                          ? ObjectAttrs
                          : Old.getAttributes().getParamAttrs(I));
    }
    IRBuilder<> Builder(&Old);
    CallInst *New = Builder.CreateCall(NewF, Args);
    New->setCallingConv(Old.getCallingConv());
    New->setAttributes(AttributeList::get(Ctx, Old.getAttributes().getFnAttrs(),
                                          Old.getAttributes().getRetAttrs(),
                                          Attrs));
    New->setMemoryEffects(Old.getMemoryEffects() | MemoryEffects::argMemOnly());
    New->setDebugLoc(Old.getDebugLoc());
    Update.CopyBack->eraseFromParent();
    Old.eraseFromParent();
    while (!Update.Result->use_empty())
      cast<Instruction>(Update.Result->user_back())->eraseFromParent();
    Update.Result->eraseFromParent();
  }
  NewF->takeName(&F);
  F.eraseFromParent();
  return true;
}

class AVMAggregateReturns final : public ModulePass {
public:
  static char ID;
  AVMAggregateReturns() : ModulePass(ID) {
    initializeAVMAggregateReturnsPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) override {
    if (DisableAggregateReturns)
      return false;
    bool Changed = false;
    for (Function &F : make_early_inc_range(M))
      Changed |= specialize(F);
    return Changed;
  }
};
} // namespace

char AVMAggregateReturns::ID = 0;
INITIALIZE_PASS(AVMAggregateReturns, DEBUG_TYPE, PASS_NAME, false, false)

ModulePass *llvm::createAVMAggregateReturnsPass() {
  return new AVMAggregateReturns();
}
