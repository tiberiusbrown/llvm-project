//===-- AVMStackTargets.cpp - Conservative stack call targets --------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "AVM.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/ValueLatticeUtils.h"
#include "llvm/BinaryFormat/AVM.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/CalledValuePropagation.h"

using namespace llvm;

static cl::opt<unsigned> TargetLimit(
    "avm-stack-target-limit", cl::Hidden, cl::init(32),
    cl::desc("Maximum functions or memory locations per AVM pointer value"));
static cl::opt<unsigned> IterationLimit(
    "avm-stack-analysis-iterations", cl::Hidden, cl::init(256),
    cl::desc("Maximum fixed-point iterations for AVM pointer analysis"));

namespace {
// Flow-insensitive, field-sensitive may-points-to analysis. A location is an
// allocation/global and a byte offset. Unknown offsets conservatively taint
// the whole object. Unknown addresses can alias escaped objects, but cannot
// alias an alloca whose address never escaped. No executable IR is changed.
using Location = std::pair<const Value *, int64_t>;
constexpr int64_t AnyOffset = INT64_MIN;
struct Pointers {
  bool Unknown = false, Limited = false;
  SmallVector<Function *, 4> Functions;
  SmallVector<Location, 4> Locations;

  static Pointers unknown(bool Limited = false) {
    Pointers P;
    P.Unknown = true;
    P.Limited = Limited;
    return P;
  }
  void retainObjects() {
    SmallVector<Location, 4> Objects;
    for (Location L : Locations) {
      L.second = AnyOffset;
      if (!is_contained(Objects, L))
        Objects.push_back(L);
    }
    Locations = std::move(Objects);
  }
  bool merge(const Pointers &Other) {
    bool Changed = false;
    if (Other.Limited && !Limited) {
      Limited = Changed = true;
    }
    if (Other.Unknown && !Unknown) {
      Unknown = Changed = true;
      retainObjects();
    }
    for (Function *F : Other.Functions)
      if (!Unknown && !is_contained(Functions, F)) {
        Functions.push_back(F);
        Changed = true;
      }
    for (Location L : Other.Locations) {
      if (Unknown)
        L.second = AnyOffset;
      if (!is_contained(Locations, L)) {
        Locations.push_back(L);
        Changed = true;
      }
    }
    if (Functions.size() > TargetLimit || Locations.size() > TargetLimit) {
      Unknown = Limited = true;
      Functions.clear();
      // Retain location provenance even at top so escapes cannot hide an
      // alias to a previously protected alloca. Top values never prove calls.
      // Collapse offsets at top: pointer induction variables must not keep
      // enumerating loop iterations after their proof limit is exhausted.
      retainObjects();
    }
    return Changed;
  }
};
struct Cell {
  Pointers Value;
  uint64_t Size;
};

class PointerAnalysis {
  Module &M;
  const DataLayout &DL;
  DenseMap<const Value *, Pointers> Values, Returns;
  DenseMap<Location, Cell> Memory;
  SmallPtrSet<const Value *, 32> Escaped, Tainted;
  SmallVector<Instruction *, 0> Instructions;
  bool Changed = false;

  void merge(const Value *V, const Pointers &P) {
    Changed |= Values[V].merge(P);
  }
  void taint(const Value *Object) {
    if (auto *G = dyn_cast<GlobalVariable>(Object); G && G->isConstant())
      return; // Writing immutable storage is undefined behavior.
    Changed |= Tainted.insert(Object).second;
  }
  void escape(const Pointers &P) {
    for (Location L : P.Locations)
      Changed |= Escaped.insert(L.first).second;
  }
  void taintEscaped() {
    for (const Value *V : Escaped)
      taint(V);
  }
  Pointers offset(Pointers P, const GEPOperator &G) {
    APInt N(DL.getIndexSizeInBits(G.getPointerAddressSpace()), 0);
    bool Constant = G.accumulateConstantOffset(DL, N);
    int64_t Delta = Constant ? N.getSExtValue() : AnyOffset;
    for (Location &L : P.Locations) {
      if (L.second == AnyOffset || Delta == AnyOffset ||
          (Delta > 0 && L.second > INT64_MAX - Delta) ||
          (Delta < 0 && L.second < INT64_MIN - Delta))
        L.second = AnyOffset;
      else
        L.second += Delta;
    }
    return P;
  }
  Pointers value(const Value *V) {
    if (isa<InlineAsm>(V))
      return Pointers::unknown();
    if (auto *F = dyn_cast<Function>(V)) {
      Pointers P;
      P.Functions.push_back(const_cast<Function *>(F));
      return P;
    }
    if (isa<GlobalVariable>(V) || isa<AllocaInst>(V)) {
      Pointers P;
      P.Locations.emplace_back(V, 0);
      return P;
    }
    if (auto *A = dyn_cast<GlobalAlias>(V))
      return A->getAliasee() ? value(A->getAliasee()) : Pointers::unknown();
    if (isa<ConstantPointerNull>(V) || isa<UndefValue>(V) || isa<PoisonValue>(V))
      return {}; // Calling null, undef, or poison is undefined behavior.
    if (auto *C = dyn_cast<ConstantExpr>(V)) {
      if (auto *G = dyn_cast<GEPOperator>(C))
        return offset(value(G->getPointerOperand()), *G);
      if (C->isCast() && C->getOperand(0)->getType()->isPointerTy() &&
          C->getType()->isPointerTy())
        return value(C->getOperand(0));
      return Pointers::unknown();
    }
    if (isa<Constant>(V))
      return Pointers::unknown();
    return Values.lookup(V);
  }
  Cell &cell(Location L, uint64_t Size) {
    auto [It, New] = Memory.try_emplace(L, Cell{{}, Size});
    Changed |= New;
    Cell &C = It->second;
    if (C.Size != Size || L.second == AnyOffset || Tainted.contains(L.first))
      Changed |= C.Value.merge(Pointers::unknown());
    if (auto *G = dyn_cast<GlobalVariable>(L.first)) {
      if (!G->hasDefinitiveInitializer() ||
          (!G->isConstant() && !G->hasLocalLinkage()) ||
          (New && !G->getInitializer()->isNullValue()))
        Changed |= C.Value.merge(Pointers::unknown());
    }
    return C;
  }
  void initialize(const Value *Object, Constant *C, uint64_t Offset) {
    Type *T = C->getType();
    if (T->isPointerTy()) {
      // Initialize directly, before an on-demand load can conservatively
      // classify an unrecognized representation as unknown.
      Memory[{Object, int64_t(Offset)}] =
          {value(C), DL.getTypeStoreSize(T).getFixedValue()};
      if (Escaped.contains(Object) &&
          !cast<GlobalVariable>(Object)->isConstant())
        Memory[{Object, int64_t(Offset)}].Value = Pointers::unknown();
      return;
    }
    if (auto *ST = dyn_cast<StructType>(T)) {
      const StructLayout *Layout = DL.getStructLayout(ST);
      for (unsigned I = 0; I < ST->getNumElements(); ++I)
        if (Constant *Elt = C->getAggregateElement(I))
          initialize(Object, Elt, Offset + Layout->getElementOffset(I));
    } else if (auto *AT = dyn_cast<ArrayType>(T)) {
      uint64_t Size = DL.getTypeAllocSize(AT->getElementType()).getFixedValue();
      for (unsigned I = 0; I < AT->getNumElements(); ++I)
        if (Constant *Elt = C->getAggregateElement(I))
          initialize(Object, Elt, Offset + I * Size);
    }
  }
  Pointers load(const Pointers &Address, uint64_t Size) {
    Pointers Result;
    if (Address.Unknown || !Address.Functions.empty())
      Result = Pointers::unknown(Address.Limited);
    for (Location L : Address.Locations) {
      if (L.second == AnyOffset)
        Result.merge(Pointers::unknown());
      else
        Result.merge(cell(L, Size).Value);
    }
    return Result;
  }
  // Byte writes/copies must overlap complete pointer cells. Partial writes or
  // differently sized pointers become unknown instead of inventing a target.
  static bool overlaps(int64_t A, uint64_t AS, int64_t B, uint64_t BS) {
    return A == AnyOffset || B == AnyOffset ||
           (A < B ? uint64_t(B) - uint64_t(A) < AS
                  : uint64_t(A) - uint64_t(B) < BS);
  }
  void write(const Pointers &Address, const Pointers *P, uint64_t Size) {
    if (Address.Unknown) {
      if (P)
        escape(*P);
      taintEscaped();
    }
    for (Location L : Address.Locations) {
      if (L.second == AnyOffset) {
        taint(L.first);
        if (P)
          escape(*P);
        continue;
      }
      if (P) {
        Changed |= cell(L, Size).Value.merge(*P);
        if (Escaped.contains(L.first))
          escape(*P);
      }
      for (auto &[Other, C] : Memory)
        if (Other.first == L.first && overlaps(L.second, Size, Other.second, C.Size) &&
            (!P || L.second != Other.second || Size != C.Size))
          Changed |= C.Value.merge(Pointers::unknown());
    }
  }
  void copy(MemTransferInst &I) {
    Pointers Dst = value(I.getDest()), Src = value(I.getSource());
    auto *N = dyn_cast<ConstantInt>(I.getLength());
    if (!N || N->getValue().getActiveBits() > 63 || I.isVolatile()) {
      write(Dst, nullptr, UINT64_MAX);
      return;
    }
    uint64_t Size = N->getZExtValue();
    if (!Size)
      return;
    if (Dst.Unknown) {
      escape(Src);
      taintEscaped();
    }
    // Snapshot keys: on-demand source/destination cells can rehash Memory.
    SmallVector<std::pair<Location, uint64_t>, 0> Cells;
    for (auto &[L, C] : Memory)
      Cells.emplace_back(L, C.Size);
    for (Location D : Dst.Locations) {
      if (D.second == AnyOffset || Src.Unknown || !Src.Functions.empty()) {
        taint(D.first);
        continue;
      }
      for (Location S : Src.Locations) {
        if (S.second == AnyOffset) {
          taint(D.first);
          continue;
        }
        // Both directions matter: a destination load can expose a previously
        // unseen source cell, while a source initializer can expose a dest cell.
        for (auto [L, Width] : Cells) {
          bool FromSource = L.first == S.first;
          if (!FromSource && L.first != D.first)
            continue;
          int64_t Begin = FromSource ? S.second : D.second;
          if (!overlaps(Begin, Size, L.second, Width))
            continue;
          if (L.second < Begin || uint64_t(L.second - Begin) > Size ||
              Width > Size - uint64_t(L.second - Begin)) {
            taint(D.first);
            continue;
          }
          int64_t Delta = L.second - Begin;
          Pointers P = cell({S.first, S.second + Delta}, Width).Value;
          Pointers Address;
          Address.Locations.emplace_back(D.first, D.second + Delta);
          write(Address, &P, Width);
        }
      }
    }
  }
  void call(CallBase &CB) {
    if (auto *MT = dyn_cast<MemTransferInst>(&CB)) {
      copy(*MT);
      return;
    }
    if (auto *MS = dyn_cast<MemSetInst>(&CB)) {
      auto *N = dyn_cast<ConstantInt>(MS->getLength());
      write(value(MS->getDest()), nullptr,
            N ? N->getLimitedValue() : UINT64_MAX);
      return;
    }
    if (auto *II = dyn_cast<IntrinsicInst>(&CB)) {
      if (II->getIntrinsicID() == Intrinsic::lifetime_start ||
          II->getIntrinsicID() == Intrinsic::lifetime_end ||
          II->getIntrinsicID() == Intrinsic::assume || isa<DbgInfoIntrinsic>(II))
        return;
    }
    Pointers Callees = value(CB.getCalledOperand());
    bool Unknown = Callees.Unknown || !Callees.Locations.empty();
    Pointers Result;
    for (Function *F : Callees.Functions) {
      if (F->isDeclaration() || !F->hasExactDefinition()) {
        Unknown = true;
        continue;
      }
      unsigned Count = std::min<unsigned>(CB.arg_size(), F->arg_size());
      for (unsigned A = 0; A < Count; ++A)
        if (F->getArg(A)->getType()->isPointerTy()) {
          if (CB.getArgOperand(A)->getType()->isPointerTy())
            merge(F->getArg(A), value(CB.getArgOperand(A)));
          else
            merge(F->getArg(A), Pointers::unknown());
        }
      if (CB.getType()->isPointerTy())
        Result.merge(Returns.lookup(F));
    }
    if (Unknown) {
      Result.merge(Pointers::unknown(Callees.Limited));
      for (Value *Arg : CB.args())
        if (Arg->getType()->isPointerTy()) {
          Pointers P = value(Arg);
          escape(P);
          if (!CB.onlyReadsMemory())
            for (Location L : P.Locations)
              taint(L.first);
        }
      if (!CB.onlyReadsMemory())
        taintEscaped();
      if (CB.isInlineAsm() && !CB.onlyReadsMemory() &&
          !cast<InlineAsm>(CB.getCalledOperand())->getAsmString().empty()) {
        // Assembly with arbitrary memory effects may name private globals or
        // access SP directly without an IR pointer operand.
        for (GlobalVariable &G : M.globals())
          taint(&G);
        for (Instruction *I : Instructions)
          if (isa<AllocaInst>(I)) {
            Changed |= Escaped.insert(I).second;
            taint(I);
          }
      }
    }
    if (CB.getType()->isPointerTy())
      merge(&CB, Result);
  }
  void instruction(Instruction &I) {
    if (auto *CB = dyn_cast<CallBase>(&I)) {
      call(*CB);
    } else if (auto *S = dyn_cast<StoreInst>(&I)) {
      Pointers Address = value(S->getPointerOperand());
      Pointers P;
      bool IsPointer = S->getValueOperand()->getType()->isPointerTy();
      if (IsPointer) {
        P = value(S->getValueOperand());
        if (S->isVolatile() || S->isAtomic())
          P.merge(Pointers::unknown());
      }
      write(Address, IsPointer ? &P : nullptr,
            DL.getTypeStoreSize(S->getValueOperand()->getType()).getFixedValue());
    } else if (auto *R = dyn_cast<ReturnInst>(&I)) {
      if (R->getReturnValue() && R->getReturnValue()->getType()->isPointerTy()) {
        Pointers P = value(R->getReturnValue());
        Changed |= Returns[R->getFunction()].merge(P);
        if (!R->getFunction()->hasLocalLinkage())
          escape(P);
      }
    } else if (isa<PtrToIntInst>(&I)) {
      escape(value(I.getOperand(0)));
    } else if (isa<AtomicRMWInst, AtomicCmpXchgInst>(&I)) {
      write(value(I.getOperand(0)), nullptr, UINT64_MAX);
      for (Value *Op : I.operands())
        if (Op->getType()->isPointerTy())
          escape(value(Op));
    } else if (I.getType()->isPointerTy()) {
      Pointers P;
      if (auto *G = dyn_cast<GetElementPtrInst>(&I)) {
        P = offset(value(G->getPointerOperand()), *cast<GEPOperator>(G));
      } else if (auto *L = dyn_cast<LoadInst>(&I)) {
        P = load(value(L->getPointerOperand()),
                 DL.getTypeStoreSize(L->getType()).getFixedValue());
        if (L->isVolatile() || L->isAtomic())
          P.merge(Pointers::unknown());
      } else if (auto *Phi = dyn_cast<PHINode>(&I)) {
        for (Value *V : Phi->incoming_values())
          P.merge(value(V));
      } else if (auto *S = dyn_cast<SelectInst>(&I)) {
        P = value(S->getTrueValue());
        P.merge(value(S->getFalseValue()));
      } else if (isa<BitCastInst, AddrSpaceCastInst>(&I)) {
        P = value(I.getOperand(0));
      } else if (isa<AllocaInst>(&I)) {
        return;
      } else {
        P = Pointers::unknown();
      }
      merge(&I, P);
    } else {
      // Pointer-bearing aggregates and unsupported operations can transport
      // addresses outside the modeled pointer SSA graph.
      for (Value *Op : I.operands())
        if (Op->getType()->isPointerTy() &&
            !isa<ICmpInst, BranchInst>(&I))
          escape(value(Op));
    }
  }

public:
  explicit PointerAnalysis(Module &M) : M(M), DL(M.getDataLayout()) {}
  bool run() {
    for (GlobalAlias &A : M.aliases())
      if (!A.hasLocalLinkage() && A.getAliasee())
        escape(value(A.getAliasee()));
    for (GlobalVariable &G : M.globals()) {
      if (!G.hasLocalLinkage() && !G.isConstant())
        Escaped.insert(&G);
      if (G.hasInitializer())
        initialize(&G, G.getInitializer(), 0);
    }
    for (Function &F : M) {
      if (!canTrackArgumentsInterprocedurally(&F))
        for (Argument &A : F.args())
          if (A.getType()->isPointerTy())
            Values[&A] = Pointers::unknown();
      for (Instruction &I : instructions(F))
        Instructions.push_back(&I);
    }
    for (unsigned Iteration = 0; Iteration < IterationLimit; ++Iteration) {
      Changed = false;
      // An escaped object's stored pointers can expose further objects. A
      // tainted object can receive arbitrary new pointers from outside IR.
      for (auto &[L, C] : Memory) {
        if (Escaped.contains(L.first))
          escape(C.Value);
        if (Tainted.contains(L.first))
          Changed |= C.Value.merge(Pointers::unknown());
      }
      for (Instruction *I : Instructions)
        instruction(*I);
      if (!Changed)
        return true;
    }
    return false; // No partial fixed point may be used as a complete proof.
  }
  Pointers targets(const Value *V) { return value(V); }
};

class AVMStackTargets final : public ModulePass {
public:
  static char ID;
  AVMStackTargets() : ModulePass(ID) {}
  bool runOnModule(Module &M) override {
    ModuleAnalysisManager AM;
    CalledValuePropagationPass().run(M, AM);
    PointerAnalysis Analysis(M);
    bool Converged = Analysis.run();
    LLVMContext &C = M.getContext();
    auto number = [&](unsigned N) {
      return MDNode::get(C, ConstantAsMetadata::get(
                                ConstantInt::get(Type::getInt32Ty(C), N)));
    };
    for (Function &F : M) {
      if (F.isDeclaration())
        continue;
      DominatorTree DT(F);
      SmallVector<Metadata *, 4> Registrations;
      for (Instruction &I : instructions(F)) {
        auto *CB = dyn_cast<CallBase>(&I);
        if (!CB)
          continue;
        auto *Direct = dyn_cast<Function>(CB->getCalledOperand()->stripPointerCasts());
        if (Direct && Direct->hasExternalWeakLinkage() &&
            !CB->getMetadata("avm.stack.dispatch")) {
          // Only a proven non-null control-flow edge permits skipping a weak
          // reference. An unguarded undefined weak call stays a gap.
          for (BasicBlock &B : F) {
            auto *Br = dyn_cast<BranchInst>(B.getTerminator());
            if (!Br || !Br->isConditional())
              continue;
            auto *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
            if (!Cmp || !Cmp->isEquality())
              continue;
            Value *L = Cmp->getOperand(0), *R = Cmp->getOperand(1);
            if (isa<ConstantPointerNull>(L))
              std::swap(L, R);
            if (L->stripPointerCasts() != Direct || !isa<ConstantPointerNull>(R))
              continue;
            unsigned NonNull = Cmp->getPredicate() == ICmpInst::ICMP_NE ? 0 : 1;
            if (DT.dominates(BasicBlockEdge(&B, Br->getSuccessor(NonNull)),
                             CB->getParent())) {
              CB->setMetadata("avm.stack.dispatch", number(AVM::StackCallGuardedWeak));
              break;
            }
          }
        }
        if (Direct && Direct->getName() == "__avm_register_local_dtor") {
          Pointers P = Converged && CB->arg_size()
                           ? Analysis.targets(CB->getArgOperand(0))
                           : Pointers::unknown();
          SmallVector<Metadata *, 4> Set;
          if (P.Unknown || !P.Locations.empty())
            Set.push_back(ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(C), 1)));
          else {
            llvm::sort(P.Functions, [](Function *A, Function *B) {
              return A->getName() < B->getName();
            });
            for (Function *Target : P.Functions)
              Set.push_back(ValueAsMetadata::get(Target));
          }
          Registrations.push_back(MDNode::get(C, Set));
        }
        if (!CB->isIndirectCall() || CB->getMetadata(LLVMContext::MD_callees) ||
            CB->getMetadata("avm.stack.dispatch"))
          continue;
        Pointers P = Analysis.targets(CB->getCalledOperand());
        if (Converged && !P.Unknown && P.Locations.empty() && !P.Functions.empty()) {
          llvm::sort(P.Functions, [](Function *A, Function *B) {
            return A->getName() < B->getName();
          });
          CB->setMetadata(LLVMContext::MD_callees, MDBuilder(C).createCallees(P.Functions));
        } else {
          CB->setMetadata("avm.stack.gap", number(
              !Converged ? AVM::StackGapIterationLimit
              : P.Limited ? AVM::StackGapTargetLimit
                          : AVM::StackGapPointerFlow));
        }
      }
      F.setMetadata("avm.stack.registrations",
                    Registrations.empty() ? nullptr : MDNode::get(C, Registrations));
    }
    return true;
  }
  StringRef getPassName() const override { return "AVM stack call targets"; }
};
char AVMStackTargets::ID = 0;
} // namespace

ModulePass *llvm::createAVMStackTargetsPass() { return new AVMStackTargets(); }
