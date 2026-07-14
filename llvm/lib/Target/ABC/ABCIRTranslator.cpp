//===-- ABCIRTranslator.cpp - Direct LLVM IR to ABC MC lowering ----------===//

#include "ABCTargetMachine.h"
#include "ABC.h"
#include "ABCSyscalls.h"
#include "MCTargetDesc/ABCMCTargetDesc.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/Pass.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>
#include <memory>
#include <optional>

using namespace llvm;

namespace {

static std::optional<unsigned> getSyscallNumber(StringRef Name) {
  return getABCSyscallNumber(Name);
}

struct StackSlot {
  unsigned Start = 0;
  unsigned Size = 0;
};

class ABCIRTranslator final : public ModulePass {
public:
  static char ID;

  ABCIRTranslator(ABCTargetMachine &TM, std::unique_ptr<MCContext> Ctx,
                  std::unique_ptr<MCObjectFileInfo> MOFI,
                  std::unique_ptr<MCStreamer> Streamer)
      : ModulePass(ID), TM(TM), Ctx(std::move(Ctx)),
        Streamer(std::move(Streamer)), OwnedMOFI(std::move(MOFI)) {}

  bool runOnModule(Module &M) override;

private:
  struct FunctionState {
    DenseMap<const Value *, StackSlot> Slots;
    DenseSet<const Instruction *> Deferred;
    DenseSet<const Instruction *> Emitted;
    DenseMap<const BasicBlock *, MCSymbol *> BlockSyms;
    unsigned ReturnBytes = 0;
    unsigned ArgBytes = 0;
    unsigned LocalBytes = 0;
    unsigned TotalBytes = 0;
    unsigned ConsumedArgBytes = 0;
  };

  ABCTargetMachine &TM;
  std::unique_ptr<MCContext> Ctx;
  std::unique_ptr<MCStreamer> Streamer;
  std::unique_ptr<MCObjectFileInfo> OwnedMOFI;
  const MCObjectFileInfo *MOFI = nullptr;
  bool Failed = false;

  bool emitGlobals(Module &M);
  bool emitGlobal(const GlobalVariable &GV);
  bool emitFunctions(Module &M);
  bool emitFunction(Function &F);
  bool buildFunctionState(Function &F, FunctionState &State);
  void findDeferredValues(Function &F, FunctionState &State);
  bool emitBasicBlock(Function &F, BasicBlock &BB, FunctionState &State);
  bool emitInstruction(Instruction &I, FunctionState &State);

  unsigned getTypeSize(Type *Ty) const;
  bool isSupportedScalarType(Type *Ty) const;
  bool isZeroInitializer(const Constant *C) const;
  bool emitConstantValue(const Constant *C, unsigned ForcedSize = 0);

  void fail(const Value *V, Twine Msg);
  void fail(Twine Msg);

  MCSymbol *getOrCreateSymbol(StringRef Name);
  MCSymbol *getBlockSymbol(const BasicBlock &BB, FunctionState &State);
  const MCExpr *symbolRef(StringRef Name) {
    return MCSymbolRefExpr::create(getOrCreateSymbol(Name), *Ctx);
  }

  void switchToText() { Streamer->switchSection(MOFI->getTextSection()); }
  void switchToBSS() { Streamer->switchSection(MOFI->getBSSSection()); }
  void switchToROData() { Streamer->switchSection(MOFI->getReadOnlySection()); }

  void emitOpcode(unsigned Opcode);
  void emitOpcodeImm(unsigned Opcode, uint64_t Imm);
  void emitOpcodeExpr(unsigned Opcode, const MCExpr *Expr);
  void emitOpcodeImmImm(unsigned Opcode, uint64_t Imm0, uint64_t Imm1);

  bool emitPushZeroBytes(unsigned Size);
  bool emitPushInteger(uint64_t Value, unsigned Size);
  bool emitLoadValue(Value *V, FunctionState &State);
  bool emitLoadSlot(const StackSlot &Slot, const FunctionState &State,
                    unsigned ExtraDepth = 0);
  bool emitStoreSlot(const StackSlot &Slot, const FunctionState &State,
                     unsigned ExtraDepth = 0);
  bool emitLoadFromPointer(Value *Ptr, Type *ValueTy, FunctionState &State);
  bool emitStoreToPointer(Value *Ptr, Value *Stored, FunctionState &State);
  bool emitGetElementPtr(GetElementPtrInst &GEP, FunctionState &State);
  bool emitPhiIncoming(BasicBlock &From, BasicBlock &To,
                       FunctionState &State);
  bool emitBinaryOperator(BinaryOperator &BO, FunctionState &State);
  bool emitICmp(ICmpInst &ICmp, FunctionState &State);
  bool emitCast(CastInst &CI, FunctionState &State);
  bool emitCall(CallBase &CB, FunctionState &State);
  bool emitReturn(ReturnInst &RI, FunctionState &State);

  unsigned slotOffset(const StackSlot &Slot, const FunctionState &State,
                      unsigned ExtraDepth = 0) const {
    return State.TotalBytes - State.ConsumedArgBytes - Slot.Start + ExtraDepth;
  }
};

} // namespace

char ABCIRTranslator::ID = 0;

ModulePass *llvm::createABCIRTranslatorPass(
    ABCTargetMachine &TM, std::unique_ptr<MCContext> Ctx,
    std::unique_ptr<MCObjectFileInfo> MOFI,
    std::unique_ptr<MCStreamer> Streamer) {
  return new ABCIRTranslator(TM, std::move(Ctx), std::move(MOFI),
                             std::move(Streamer));
}

unsigned ABCIRTranslator::getTypeSize(Type *Ty) const {
  if (!Ty || Ty->isVoidTy())
    return 0;
  if (Ty->isIntegerTy(1))
    return 1;
  if (Ty->isIntegerTy()) {
    unsigned Bytes = Ty->getIntegerBitWidth() / 8;
    return Bytes;
  }
  if (Ty->isFloatTy())
    return 4;
  if (Ty->isPointerTy())
    return TM.createDataLayout().getPointerSize(Ty->getPointerAddressSpace());
  if (auto *AT = dyn_cast<ArrayType>(Ty))
    return getTypeSize(AT->getElementType()) * AT->getNumElements();
  return 0;
}

bool ABCIRTranslator::isSupportedScalarType(Type *Ty) const {
  if (!Ty)
    return false;
  if (Ty->isPointerTy()) {
    unsigned Size = getTypeSize(Ty);
    return Size == 2 || Size == 3;
  }
  unsigned Size = getTypeSize(Ty);
  return Size == 1 || Size == 2 || Size == 3 || Size == 4;
}

bool ABCIRTranslator::isZeroInitializer(const Constant *C) const {
  if (!C)
    return false;
  if (isa<ConstantAggregateZero>(C) || isa<ConstantPointerNull>(C) ||
      isa<UndefValue>(C))
    return true;
  if (auto *CI = dyn_cast<ConstantInt>(C))
    return CI->isZero();
  if (auto *CDS = dyn_cast<ConstantDataSequential>(C)) {
    for (char Byte : CDS->getRawDataValues())
      if (Byte != 0)
        return false;
    return true;
  }
  if (auto *CA = dyn_cast<ConstantArray>(C)) {
    for (const Use &U : CA->operands())
      if (!isZeroInitializer(cast<Constant>(U.get())))
        return false;
    return true;
  }
  return false;
}

void ABCIRTranslator::fail(const Value *V, Twine Msg) {
  Failed = true;
  if (auto *I = dyn_cast_or_null<Instruction>(V))
    I->getContext().emitError(I, Msg);
  else if (auto *F = dyn_cast_or_null<Function>(V))
    errs() << "abc target error: function \"" << F->getName()
           << "\": " << Msg << "\n";
  else
    errs() << "abc target error: " << Msg << "\n";
}

void ABCIRTranslator::fail(Twine Msg) {
  Failed = true;
  errs() << "abc target error: " << Msg << "\n";
}

MCSymbol *ABCIRTranslator::getOrCreateSymbol(StringRef Name) {
  return Ctx->getOrCreateSymbol(Name);
}

MCSymbol *ABCIRTranslator::getBlockSymbol(const BasicBlock &BB,
                                          FunctionState &State) {
  MCSymbol *&Sym = State.BlockSyms[&BB];
  if (!Sym) {
    StringRef FnName = BB.getParent()->getName();
    if (BB.hasName()) {
      Sym = Ctx->getOrCreateSymbol(Twine(FnName) + "." + BB.getName());
    } else {
      unsigned Index = State.BlockSyms.size() - 1;
      Sym = Ctx->getOrCreateSymbol(Twine(FnName) + ".bb" + Twine(Index));
    }
  }
  return Sym;
}

void ABCIRTranslator::emitOpcode(unsigned Opcode) {
  MCInst Inst;
  Inst.setOpcode(Opcode);
  Streamer->emitInstruction(Inst, TM.getMCSubtargetInfo());
}

void ABCIRTranslator::emitOpcodeImm(unsigned Opcode, uint64_t Imm) {
  MCInst Inst;
  Inst.setOpcode(Opcode);
  Inst.addOperand(MCOperand::createImm(Imm));
  Streamer->emitInstruction(Inst, TM.getMCSubtargetInfo());
}

void ABCIRTranslator::emitOpcodeExpr(unsigned Opcode, const MCExpr *Expr) {
  MCInst Inst;
  Inst.setOpcode(Opcode);
  Inst.addOperand(MCOperand::createExpr(Expr));
  Streamer->emitInstruction(Inst, TM.getMCSubtargetInfo());
}

void ABCIRTranslator::emitOpcodeImmImm(unsigned Opcode, uint64_t Imm0,
                                       uint64_t Imm1) {
  MCInst Inst;
  Inst.setOpcode(Opcode);
  Inst.addOperand(MCOperand::createImm(Imm0));
  Inst.addOperand(MCOperand::createImm(Imm1));
  Streamer->emitInstruction(Inst, TM.getMCSubtargetInfo());
}

bool ABCIRTranslator::emitPushZeroBytes(unsigned Size) {
  switch (Size) {
  case 1:
    emitOpcode(ABC::P0);
    return true;
  case 2:
    emitOpcode(ABC::P00);
    return true;
  case 3:
    emitOpcode(ABC::P000);
    return true;
  case 4:
    emitOpcode(ABC::P0000);
    return true;
  default:
    fail("abc target does not support zero fill of this size");
    return false;
  }
}

bool ABCIRTranslator::emitPushInteger(uint64_t Value, unsigned Size) {
  switch (Size) {
  case 1:
    if (Value <= 8) {
      emitOpcode(ABC::P0 + unsigned(Value));
      return true;
    }
    if (Value == 16) {
      emitOpcode(ABC::P16);
      return true;
    }
    if (Value == 32) {
      emitOpcode(ABC::P32);
      return true;
    }
    if (Value == 64) {
      emitOpcode(ABC::P64);
      return true;
    }
    if (Value == 128) {
      emitOpcode(ABC::P128);
      return true;
    }
    emitOpcodeImm(ABC::PUSH, Value & 0xff);
    return true;
  case 2:
    if (Value == 0)
      return emitPushZeroBytes(2);
    emitOpcodeImm(ABC::PUSH2, Value & 0xffff);
    return true;
  case 3:
    if (Value == 0)
      return emitPushZeroBytes(3);
    emitOpcodeImm(ABC::PUSH3, Value & 0xffffff);
    return true;
  case 4:
    if (Value == 0)
      return emitPushZeroBytes(4);
    emitOpcodeImm(ABC::PUSH4, Value);
    return true;
  default:
    fail("abc target only supports 1, 2, and 4 byte integer constants");
    return false;
  }
}

bool ABCIRTranslator::emitConstantValue(const Constant *C, unsigned ForcedSize) {
  unsigned Size = ForcedSize ? ForcedSize : getTypeSize(C->getType());
  if (isa<UndefValue>(C) || isa<ConstantAggregateZero>(C) ||
      isa<ConstantPointerNull>(C))
    return emitPushZeroBytes(Size);
  if (auto *CI = dyn_cast<ConstantInt>(C))
    return emitPushInteger(CI->getZExtValue(), Size);
  if (auto *GV = dyn_cast<GlobalValue>(C->stripPointerCasts())) {
    unsigned AddrSpace = GV->getAddressSpace();
    if (AddrSpace == 0) {
      emitOpcodeExpr(ABC::PUSHG, symbolRef(GV->getName()));
      return true;
    }
    if (AddrSpace == 1) {
      emitOpcodeExpr(ABC::PUSHL, symbolRef(GV->getName()));
      return true;
    }
  }
  if (auto *CE = dyn_cast<ConstantExpr>(C)) {
    const DataLayout DL = TM.createDataLayout();
    const Constant *Base = CE;
    auto *PtrTy = dyn_cast<PointerType>(CE->getType());
    unsigned OffsetBits =
        PtrTy ? DL.getIndexTypeSizeInBits(PtrTy) : DL.getPointerSizeInBits();
    APInt Offset(OffsetBits, 0, true);
    while (auto *BaseCE = dyn_cast<ConstantExpr>(Base)) {
      if (auto *GEP = dyn_cast<GEPOperator>(BaseCE)) {
        if (!GEP->accumulateConstantOffset(DL, Offset))
          break;
        Base = BaseCE->getOperand(0);
        continue;
      }
      if (BaseCE->isCast()) {
        Base = BaseCE->getOperand(0);
        continue;
      }
      break;
    }
    if (auto *GV = dyn_cast<GlobalValue>(Base->stripPointerCasts())) {
      if (GV->getAddressSpace() == 0) {
        emitOpcodeExpr(
            ABC::PUSHG,
            MCBinaryExpr::createAdd(symbolRef(GV->getName()),
                                    MCConstantExpr::create(Offset.getSExtValue(), *Ctx),
                                    *Ctx));
        return true;
      }
      if (GV->getAddressSpace() == 1) {
        emitOpcodeExpr(
            ABC::PUSHL,
            MCBinaryExpr::createAdd(symbolRef(GV->getName()),
                                    MCConstantExpr::create(Offset.getSExtValue(), *Ctx),
                                    *Ctx));
        return true;
      }
    }
  }
  fail(C, "abc target does not support this constant");
  return false;
}

bool ABCIRTranslator::emitLoadSlot(const StackSlot &Slot,
                                   const FunctionState &State,
                                   unsigned ExtraDepth) {
  unsigned Off = slotOffset(Slot, State, ExtraDepth);
  switch (Slot.Size) {
  case 1:
    emitOpcodeImm(ABC::GETL, Off);
    return true;
  case 2:
    emitOpcodeImm(ABC::GETL2, Off);
    return true;
  case 4:
    emitOpcodeImm(ABC::GETL4, Off);
    return true;
  default:
    emitOpcodeImmImm(ABC::GETLN, Slot.Size, Off);
    return true;
  }
}

bool ABCIRTranslator::emitStoreSlot(const StackSlot &Slot,
                                    const FunctionState &State,
                                    unsigned ExtraDepth) {
  unsigned Off = slotOffset(Slot, State, ExtraDepth);
  switch (Slot.Size) {
  case 1:
    emitOpcodeImm(ABC::SETL, Off);
    return true;
  case 2:
    emitOpcodeImm(ABC::SETL2, Off);
    return true;
  case 4:
    emitOpcodeImm(ABC::SETL4, Off);
    return true;
  default:
    emitOpcodeImmImm(ABC::SETLN, Slot.Size, Off);
    return true;
  }
}

bool ABCIRTranslator::emitLoadValue(Value *V, FunctionState &State) {
  if (auto *C = dyn_cast<Constant>(V))
    return emitConstantValue(C);

  if (auto *I = dyn_cast<Instruction>(V)) {
    if (State.Deferred.contains(I)) {
      if (!State.Emitted.insert(I).second) {
        fail(I, "abc target attempted to emit a stack value more than once");
        return false;
      }
      return emitInstruction(*I, State);
    }
  }

  auto It = State.Slots.find(V);
  if (It != State.Slots.end())
    return emitLoadSlot(It->second, State);

  fail(V, "abc target does not support using this value directly");
  return false;
}

bool ABCIRTranslator::emitLoadFromPointer(Value *Ptr, Type *ValueTy,
                                          FunctionState &State) {
  unsigned Size = getTypeSize(ValueTy);
  if (auto *AI = dyn_cast<AllocaInst>(Ptr))
    return emitLoadSlot(State.Slots.lookup(AI), State);
  if (auto *GV = dyn_cast<GlobalVariable>(Ptr)) {
    if (!isSupportedScalarType(ValueTy)) {
      fail(GV, "abc target only supports scalar global loads in v1");
      return false;
    }
    switch (Size) {
    case 1:
      emitOpcodeExpr(ABC::GETG, symbolRef(GV->getName()));
      return true;
    case 2:
      emitOpcodeExpr(ABC::GETG2, symbolRef(GV->getName()));
      return true;
    case 4:
      emitOpcodeExpr(ABC::GETG4, symbolRef(GV->getName()));
      return true;
    default:
      return false;
    }
  }

  if (auto *PtrTy = dyn_cast<PointerType>(Ptr->getType())) {
    if (!emitLoadValue(Ptr, State))
      return false;
    if (PtrTy->getAddressSpace() == 1) {
      if (Size == 1)
        emitOpcode(ABC::GETP);
      else
        emitOpcodeImm(ABC::GETPN, Size);
      return true;
    }
    if (Size == 1)
      emitOpcode(ABC::GETR);
    else if (Size == 2)
      emitOpcode(ABC::GETR2);
    else
      emitOpcodeImm(ABC::GETRN, Size);
    return true;
  }

  fail(Ptr, "abc target does not support this pointer-based load");
  return false;
}

bool ABCIRTranslator::emitStoreToPointer(Value *Ptr, Value *Stored,
                                         FunctionState &State) {
  unsigned Size = getTypeSize(Stored->getType());
  if (!emitLoadValue(Stored, State))
    return false;

  if (auto *AI = dyn_cast<AllocaInst>(Ptr))
    return emitStoreSlot(State.Slots.lookup(AI), State, Size);
  if (auto *GV = dyn_cast<GlobalVariable>(Ptr)) {
    switch (Size) {
    case 1:
      emitOpcodeExpr(ABC::SETG, symbolRef(GV->getName()));
      return true;
    case 2:
      emitOpcodeExpr(ABC::SETG2, symbolRef(GV->getName()));
      return true;
    case 4:
      emitOpcodeExpr(ABC::SETG4, symbolRef(GV->getName()));
      return true;
    default:
      fail(GV, "abc target only supports 1, 2, and 4 byte global stores");
      return false;
    }
  }

  if (auto *PtrTy = dyn_cast<PointerType>(Ptr->getType())) {
    if (!emitLoadValue(Ptr, State))
      return false;
    if (PtrTy->getAddressSpace() != 0) {
      fail(Ptr, "abc target does not support stores through this pointer");
      return false;
    }
    if (Size == 1)
      emitOpcode(ABC::SETR);
    else if (Size == 2)
      emitOpcode(ABC::SETR2);
    else
      emitOpcodeImm(ABC::SETRN, Size);
    return true;
  }

  fail(Ptr, "abc target does not support this pointer-based store");
  return false;
}

bool ABCIRTranslator::emitGetElementPtr(GetElementPtrInst &GEP,
                                        FunctionState &State) {
  if (GEP.getNumIndices() != 1) {
    fail(&GEP, "abc target only supports one-dimensional GEPs in v1");
    return false;
  }

  unsigned PointerSize = getTypeSize(GEP.getType());
  unsigned IndexSize = getTypeSize(GEP.getOperand(1)->getType());
  unsigned ElementSize = getTypeSize(GEP.getSourceElementType());
  if ((PointerSize != 2 && PointerSize != 3) ||
      (IndexSize < 1 || IndexSize > PointerSize) ||
      ElementSize == 0 || ElementSize > 0xffff) {
    fail(&GEP, "abc target does not support this GEP operand size");
    return false;
  }

  if (!emitLoadValue(GEP.getPointerOperand(), State) ||
      !emitLoadValue(GEP.getOperand(1), State))
    return false;

  // Widen the index to the pointer width before doing the byte-address
  // arithmetic. The zero bytes are pushed above the index's low bytes.
  if (IndexSize < PointerSize && !emitPushZeroBytes(PointerSize - IndexSize))
    return false;
  if (ElementSize != 1) {
    if (!emitPushInteger(ElementSize, PointerSize))
      return false;
    emitOpcode(PointerSize == 2 ? ABC::MUL2 : ABC::MUL3);
  }
  emitOpcode(PointerSize == 2 ? ABC::ADD2 : ABC::ADD3);

  if (State.Deferred.contains(&GEP))
    return true;
  if (GEP.use_empty()) {
    emitOpcodeImm(ABC::POPN, PointerSize);
    return true;
  }
  return emitStoreSlot(State.Slots.lookup(&GEP), State, PointerSize);
}

bool ABCIRTranslator::emitPhiIncoming(BasicBlock &From, BasicBlock &To,
                                      FunctionState &State) {
  for (Instruction &I : To) {
    auto *Phi = dyn_cast<PHINode>(&I);
    if (!Phi)
      break;

    int Incoming = Phi->getBasicBlockIndex(&From);
    if (Incoming < 0 ||
        !emitLoadValue(Phi->getIncomingValue(unsigned(Incoming)), State) ||
        !emitStoreSlot(State.Slots.lookup(Phi), State,
                       getTypeSize(Phi->getType())))
      return false;
  }
  return true;
}

bool ABCIRTranslator::emitBinaryOperator(BinaryOperator &BO,
                                         FunctionState &State) {
  unsigned Size = getTypeSize(BO.getType());
  bool ConsumeIncomingArgs = false;
  if (State.ReturnBytes == 0 &&
      BO.getParent()->getParent()->size() == 1 && State.LocalBytes == 0 &&
      State.ConsumedArgBytes == 0 && BO.getParent()->getParent()->arg_size() == 2 &&
      BO.isCommutative()) {
    auto *A0 = dyn_cast<Argument>(BO.getOperand(0));
    auto *A1 = dyn_cast<Argument>(BO.getOperand(1));
    ConsumeIncomingArgs = A0 && A1 && A0 != A1 && A0->hasOneUse() &&
                          A1->hasOneUse() &&
                          getTypeSize(A0->getType()) + getTypeSize(A1->getType()) ==
                              State.ArgBytes;
  }
  if (ConsumeIncomingArgs) {
    State.ConsumedArgBytes = State.ArgBytes;
  } else if (!emitLoadValue(BO.getOperand(0), State) ||
             !emitLoadValue(BO.getOperand(1), State)) {
    return false;
  }

  switch (BO.getOpcode()) {
  case Instruction::Add:
    emitOpcode(Size == 1 ? ABC::ADD : Size == 2 ? ABC::ADD2 : ABC::ADD4);
    break;
  case Instruction::Sub:
    emitOpcode(Size == 1 ? ABC::SUB : Size == 2 ? ABC::SUB2 : ABC::SUB4);
    break;
  case Instruction::Mul:
    emitOpcode(Size == 1 ? ABC::MUL : Size == 2 ? ABC::MUL2 : ABC::MUL4);
    break;
  case Instruction::And:
    emitOpcode(Size == 1 ? ABC::AND : Size == 2 ? ABC::AND2 : ABC::AND4);
    break;
  case Instruction::Or:
    emitOpcode(Size == 1 ? ABC::OR : Size == 2 ? ABC::OR2 : ABC::OR4);
    break;
  case Instruction::Xor:
    emitOpcode(Size == 1 ? ABC::XOR : Size == 2 ? ABC::XOR2 : ABC::XOR4);
    break;
  case Instruction::Shl:
    emitOpcode(Size == 1 ? ABC::LSL : Size == 2 ? ABC::LSL2 : ABC::LSL4);
    break;
  case Instruction::LShr:
    emitOpcode(Size == 1 ? ABC::LSR : Size == 2 ? ABC::LSR2 : ABC::LSR4);
    break;
  case Instruction::AShr:
    emitOpcode(Size == 1 ? ABC::ASR : Size == 2 ? ABC::ASR2 : ABC::ASR4);
    break;
  default:
    fail(&BO, "abc target does not support this arithmetic operation");
    return false;
  }

  if (State.Deferred.contains(&BO))
    return true;
  if (BO.use_empty()) {
    emitOpcodeImm(ABC::POPN, Size);
    return true;
  }
  return emitStoreSlot(State.Slots.lookup(&BO), State, Size);
}

bool ABCIRTranslator::emitICmp(ICmpInst &ICmp, FunctionState &State) {
  unsigned Size = getTypeSize(ICmp.getOperand(0)->getType());
  bool ReverseOperands =
      ICmp.getPredicate() == CmpInst::ICMP_UGT ||
      ICmp.getPredicate() == CmpInst::ICMP_ULE ||
      ICmp.getPredicate() == CmpInst::ICMP_SGT ||
      ICmp.getPredicate() == CmpInst::ICMP_SLE;
  Value *LHS = ICmp.getOperand(ReverseOperands ? 1 : 0);
  Value *RHS = ICmp.getOperand(ReverseOperands ? 0 : 1);
  if (!emitLoadValue(LHS, State) || !emitLoadValue(RHS, State))
    return false;

  auto EmitBoolFromDiff = [&]() {
    emitOpcode(Size == 1 ? ABC::SUB : Size == 2 ? ABC::SUB2 : ABC::SUB4);
    emitOpcode(Size == 1 ? ABC::BOOL : Size == 2 ? ABC::BOOL2 : ABC::BOOL4);
  };
  auto EmitUnsignedLT = [&]() {
    emitOpcode(Size == 1 ? ABC::CULT : Size == 2 ? ABC::CULT2 : ABC::CULT4);
  };
  auto EmitSignedLT = [&]() {
    emitOpcode(Size == 1 ? ABC::CSLT : Size == 2 ? ABC::CSLT2 : ABC::CSLT4);
  };

  switch (ICmp.getPredicate()) {
  case CmpInst::ICMP_EQ:
    EmitBoolFromDiff();
    emitOpcode(ABC::NOT);
    break;
  case CmpInst::ICMP_NE:
    EmitBoolFromDiff();
    break;
  case CmpInst::ICMP_ULT:
    EmitUnsignedLT();
    break;
  case CmpInst::ICMP_UGT:
    EmitUnsignedLT();
    break;
  case CmpInst::ICMP_ULE:
    EmitUnsignedLT();
    emitOpcode(ABC::NOT);
    break;
  case CmpInst::ICMP_UGE:
    EmitUnsignedLT();
    emitOpcode(ABC::NOT);
    break;
  case CmpInst::ICMP_SLT:
    EmitSignedLT();
    break;
  case CmpInst::ICMP_SGT:
    EmitSignedLT();
    break;
  case CmpInst::ICMP_SLE:
    EmitSignedLT();
    emitOpcode(ABC::NOT);
    break;
  case CmpInst::ICMP_SGE:
    EmitSignedLT();
    emitOpcode(ABC::NOT);
    break;
  default:
    fail(&ICmp, "abc target does not support this integer comparison");
    return false;
  }

  if (State.Deferred.contains(&ICmp))
    return true;
  if (ICmp.use_empty()) {
    emitOpcode(ABC::POP);
    return true;
  }
  return emitStoreSlot(State.Slots.lookup(&ICmp), State, 1);
}

bool ABCIRTranslator::emitCast(CastInst &CI, FunctionState &State) {
  unsigned SrcSize = getTypeSize(CI.getSrcTy());
  unsigned DstSize = getTypeSize(CI.getDestTy());
  if (!emitLoadValue(CI.getOperand(0), State))
    return false;

  if (SrcSize == DstSize) {
    if (State.Deferred.contains(&CI))
      return true;
    if (CI.use_empty()) {
      emitOpcodeImm(ABC::POPN, DstSize);
      return true;
    }
    return emitStoreSlot(State.Slots.lookup(&CI), State, DstSize);
  }

  if (isa<TruncInst>(&CI) && SrcSize > DstSize) {
    emitOpcodeImm(ABC::POPN, SrcSize - DstSize);
    if (State.Deferred.contains(&CI))
      return true;
    if (CI.use_empty()) {
      emitOpcodeImm(ABC::POPN, DstSize);
      return true;
    }
    return emitStoreSlot(State.Slots.lookup(&CI), State, DstSize);
  }

  if ((isa<ZExtInst>(&CI) || isa<SExtInst>(&CI)) && SrcSize < DstSize) {
    if (isa<SExtInst>(&CI) && SrcSize == 1 && DstSize == 2)
      emitOpcode(ABC::SEXT);
    else if (isa<SExtInst>(&CI) && SrcSize == 2 && DstSize == 4)
      emitOpcode(ABC::SEXT2);
    else if (isa<SExtInst>(&CI) && SrcSize == 1 && DstSize == 4)
      emitOpcode(ABC::SEXT3);
    else if (isa<ZExtInst>(&CI))
      emitPushZeroBytes(DstSize - SrcSize);
    else {
      fail(&CI, "abc target does not support this sign extension");
      return false;
    }
    if (State.Deferred.contains(&CI))
      return true;
    if (CI.use_empty()) {
      emitOpcodeImm(ABC::POPN, DstSize);
      return true;
    }
    return emitStoreSlot(State.Slots.lookup(&CI), State, DstSize);
  }

  fail(&CI, "abc target does not support this cast");
  return false;
}

bool ABCIRTranslator::emitCall(CallBase &CB, FunctionState &State) {
  Function *Callee = dyn_cast<Function>(CB.getCalledOperand()->stripPointerCasts());
  if (!Callee) {
    fail(&CB, "abc target does not support indirect calls in v1");
    return false;
  }
  if (CB.isInlineAsm()) {
    fail(&CB, "abc target does not support inline asm");
    return false;
  }
  unsigned RetSize = getTypeSize(CB.getType());

  for (int I = int(CB.arg_size()) - 1; I >= 0; --I)
    if (!emitLoadValue(CB.getArgOperand(unsigned(I)), State))
      return false;

  if (Callee->getIntrinsicID() != Intrinsic::not_intrinsic) {
    fail(&CB, "abc target does not support this LLVM intrinsic call");
    return false;
  }

  if (std::optional<unsigned> Sys = getSyscallNumber(Callee->getName())) {
    // The VM reserves the low bit of the SYS immediate, so encode the
    // source-level sysfunc number in the instruction's shifted form.
    emitOpcodeImm(ABC::SYS, *Sys << 1);
    if (State.Deferred.contains(&CB))
      return true;
    if (!CB.getType()->isVoidTy() && !CB.use_empty())
      return emitStoreSlot(State.Slots.lookup(&CB), State, RetSize);
    if (!CB.getType()->isVoidTy())
      emitOpcodeImm(ABC::POPN, RetSize);
    return true;
  }

  emitOpcodeExpr(ABC::CALL, symbolRef(Callee->getName()));
  if (State.Deferred.contains(&CB))
    return true;
  if (!CB.getType()->isVoidTy() && !CB.use_empty())
    return emitStoreSlot(State.Slots.lookup(&CB), State, RetSize);
  if (!CB.getType()->isVoidTy())
    emitOpcodeImm(ABC::POPN, RetSize);
  return true;
}

bool ABCIRTranslator::emitReturn(ReturnInst &RI, FunctionState &State) {
  if (Value *RV = RI.getReturnValue()) {
    if (!emitLoadValue(RV, State))
      return false;
    StackSlot RetSlot{State.ArgBytes + State.ReturnBytes, State.ReturnBytes};
    if (!emitStoreSlot(RetSlot, State))
      return false;
  }

  unsigned PopBytes = State.LocalBytes + State.ArgBytes - State.ConsumedArgBytes;
  if (PopBytes)
    emitOpcodeImm(ABC::POPN, PopBytes);
  emitOpcode(ABC::RET);
  return true;
}

bool ABCIRTranslator::emitInstruction(Instruction &I, FunctionState &State) {
  if (isa<DbgInfoIntrinsic>(I) || isa<AllocaInst>(I))
    return true;
  if (isa<PHINode>(&I))
    return true;

  if (auto *II = dyn_cast<IntrinsicInst>(&I)) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::lifetime_start:
    case Intrinsic::lifetime_end:
    case Intrinsic::assume:
      return true;
    case Intrinsic::smin:
    case Intrinsic::smax:
    case Intrinsic::umin:
    case Intrinsic::umax: {
      // At optimization levels, Clang canonicalizes source-level min/max
      // expressions to llvm.{s,u}{min,max}. Lower them here so the ABC
      // backend does not need to support these intrinsics as VM operations.
      if (II->arg_size() != 2 || !isSupportedScalarType(II->getType())) {
        fail(&I, "abc target only supports scalar llvm min/max intrinsics");
        return false;
      }

      unsigned Size = getTypeSize(II->getType());
      if (!emitLoadValue(II->getArgOperand(0), State) ||
          !emitLoadValue(II->getArgOperand(1), State))
        return false;

      unsigned CompareOpcode;
      bool IsSigned = II->getIntrinsicID() == Intrinsic::smin ||
                      II->getIntrinsicID() == Intrinsic::smax;
      bool IsMax = II->getIntrinsicID() == Intrinsic::smax ||
                   II->getIntrinsicID() == Intrinsic::umax;
      if (IsSigned) {
        CompareOpcode = Size == 1   ? ABC::CSLT
                        : Size == 2 ? ABC::CSLT2
                        : Size == 3 ? ABC::CSLT3
                                     : ABC::CSLT4;
      } else {
        CompareOpcode = Size == 1   ? ABC::CULT
                        : Size == 2 ? ABC::CULT2
                        : Size == 3 ? ABC::CULT3
                                     : ABC::CULT4;
      }
      emitOpcode(CompareOpcode);

      MCSymbol *Less = Ctx->createTempSymbol();
      MCSymbol *Done = Ctx->createTempSymbol();
      emitOpcodeExpr(ABC::BNZ,
                     MCSymbolRefExpr::create(Less, *Ctx));
      if (!emitLoadValue(II->getArgOperand(IsMax ? 0 : 1), State))
        return false;
      emitOpcodeExpr(ABC::JMP, MCSymbolRefExpr::create(Done, *Ctx));
      Streamer->emitLabel(Less);
      if (!emitLoadValue(II->getArgOperand(IsMax ? 1 : 0), State))
        return false;
      Streamer->emitLabel(Done);

      if (State.Deferred.contains(&I))
        return true;
      if (I.use_empty()) {
        emitOpcodeImm(ABC::POPN, Size);
        return true;
      }
      return emitStoreSlot(State.Slots.lookup(&I), State, Size);
    }
    default:
      fail(&I, "abc target does not support this LLVM intrinsic");
      return false;
    }
  }

  if (auto *SI = dyn_cast<SelectInst>(&I)) {
    unsigned Size = getTypeSize(SI->getType());
    if (!isSupportedScalarType(SI->getType()) ||
        !emitLoadValue(SI->getCondition(), State))
      return false;

    MCSymbol *TrueValue = Ctx->createTempSymbol();
    MCSymbol *Done = Ctx->createTempSymbol();
    emitOpcodeExpr(ABC::BNZ, MCSymbolRefExpr::create(TrueValue, *Ctx));
    if (!emitLoadValue(SI->getFalseValue(), State))
      return false;
    emitOpcodeExpr(ABC::JMP, MCSymbolRefExpr::create(Done, *Ctx));
    Streamer->emitLabel(TrueValue);
    if (!emitLoadValue(SI->getTrueValue(), State))
      return false;
    Streamer->emitLabel(Done);

    if (State.Deferred.contains(&I))
      return true;
    if (I.use_empty()) {
      emitOpcodeImm(ABC::POPN, Size);
      return true;
    }
    return emitStoreSlot(State.Slots.lookup(&I), State, Size);
  }

  if (auto *LI = dyn_cast<LoadInst>(&I)) {
    if (!emitLoadFromPointer(LI->getPointerOperand(), LI->getType(), State))
      return false;
    if (State.Deferred.contains(LI))
      return true;
    if (LI->use_empty()) {
      emitOpcodeImm(ABC::POPN, getTypeSize(LI->getType()));
      return true;
    }
    return emitStoreSlot(State.Slots.lookup(LI), State, getTypeSize(LI->getType()));
  }
  if (auto *SI = dyn_cast<StoreInst>(&I))
    return emitStoreToPointer(SI->getPointerOperand(), SI->getValueOperand(),
                              State);
  if (auto *GEP = dyn_cast<GetElementPtrInst>(&I))
    return emitGetElementPtr(*GEP, State);
  if (auto *BO = dyn_cast<BinaryOperator>(&I))
    return emitBinaryOperator(*BO, State);
  if (auto *ICmp = dyn_cast<ICmpInst>(&I))
    return emitICmp(*ICmp, State);
  if (auto *CI = dyn_cast<CastInst>(&I))
    return emitCast(*CI, State);
  if (auto *CB = dyn_cast<CallBase>(&I))
    return emitCall(*CB, State);
  if (auto *RI = dyn_cast<ReturnInst>(&I))
    return emitReturn(*RI, State);
  if (auto *Br = dyn_cast<UncondBrInst>(&I)) {
    if (!emitPhiIncoming(*Br->getParent(), *Br->getSuccessor(0), State))
      return false;
    emitOpcodeExpr(ABC::JMP, MCSymbolRefExpr::create(
                                 getBlockSymbol(*Br->getSuccessor(0), State),
                                 *Ctx));
    return true;
  }
  if (auto *Br = dyn_cast<CondBrInst>(&I)) {
    if (!emitPhiIncoming(*Br->getParent(), *Br->getSuccessor(0), State) ||
        !emitLoadValue(Br->getCondition(), State))
      return false;
    emitOpcodeExpr(ABC::BNZ, MCSymbolRefExpr::create(
                                 getBlockSymbol(*Br->getSuccessor(0), State),
                                 *Ctx));
    if (!emitPhiIncoming(*Br->getParent(), *Br->getSuccessor(1), State))
      return false;
    emitOpcodeExpr(ABC::JMP, MCSymbolRefExpr::create(
                                 getBlockSymbol(*Br->getSuccessor(1), State),
                                 *Ctx));
    return true;
  }

  fail(&I, "abc target does not support this instruction");
  return false;
}

bool ABCIRTranslator::buildFunctionState(Function &F, FunctionState &State) {
  if (F.isVarArg()) {
    fail(&F, "abc target does not support varargs");
    return false;
  }

  State.ReturnBytes = getTypeSize(F.getReturnType());
  if (!F.getReturnType()->isVoidTy() && !isSupportedScalarType(F.getReturnType())) {
    fail(&F, "abc target only supports scalar return values in v1");
    return false;
  }

  // The return slot is at the bottom of the call frame. Arguments are laid
  // out in reverse declaration order so the first source argument is closest
  // to the top of the data stack at callee entry.
  for (Argument &Arg : F.args())
    State.ArgBytes += getTypeSize(Arg.getType());

  unsigned Cursor = State.ArgBytes;
  for (Argument &Arg : F.args()) {
    unsigned Size = getTypeSize(Arg.getType());
    if (!isSupportedScalarType(Arg.getType())) {
      fail(&Arg, "abc target only supports 1, 2, and 4 byte scalar arguments");
      return false;
    }
    State.Slots[&Arg] = {Cursor, Size};
    Cursor -= Size;
  }

  findDeferredValues(F, State);

  unsigned LocalCursor = State.ReturnBytes + State.ArgBytes;
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (auto *AI = dyn_cast<AllocaInst>(&I)) {
        if (AI->use_empty())
          continue;
        ConstantInt *Count = dyn_cast<ConstantInt>(AI->getArraySize());
        unsigned ElemSize = getTypeSize(AI->getAllocatedType());
        if (!Count || !Count->equalsInt(1) || !isSupportedScalarType(AI->getAllocatedType())) {
          fail(AI, "abc target only supports scalar static allocas in v1");
          return false;
        }
        State.Slots[AI] = {LocalCursor, ElemSize};
        LocalCursor += ElemSize;
        continue;
      }

      if (I.getType()->isVoidTy() || I.use_empty() || State.Deferred.contains(&I))
        continue;
      if (!isSupportedScalarType(I.getType())) {
        fail(&I, "abc target only supports 1, 2, 3, and 4 byte scalar SSA values in v1");
        return false;
      }
      unsigned Size = getTypeSize(I.getType());
      State.Slots[&I] = {LocalCursor, Size};
      LocalCursor += Size;
    }
  }

  State.LocalBytes = LocalCursor - (State.ReturnBytes + State.ArgBytes);
  State.TotalBytes = LocalCursor;
  if (State.TotalBytes > 255) {
    fail(&F, "abc target frame exceeds 255 bytes");
    return false;
  }
  return true;
}

void ABCIRTranslator::findDeferredValues(Function &F, FunctionState &State) {
  DenseMap<const Instruction *, const Instruction *> Roots;

  auto CandidateRoot = [&](Instruction &I) -> const Instruction * {
    if (I.getType()->isVoidTy() || !I.hasOneUse() || isa<PHINode>(I) ||
        isa<AllocaInst>(I))
      return nullptr;
    auto *UserI = dyn_cast<Instruction>(*I.user_begin());
    if (!UserI || UserI->getParent() != I.getParent())
      return nullptr;
    const Instruction *Root = UserI;
    while (!Root->getType()->isVoidTy() && Root->hasOneUse()) {
      auto *Next = dyn_cast<Instruction>(*Root->user_begin());
      if (!Next || Next->getParent() != I.getParent())
        break;
      Root = Next;
    }
    return Root;
  };

  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if (const Instruction *Root = CandidateRoot(I))
        Roots[&I] = Root;

  // A deferred producer may only cross instructions belonging to the same
  // expression region.  This keeps calls and memory effects in LLVM IR order.
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      auto It = Roots.find(&I);
      if (It == Roots.end())
        continue;
      const Instruction *Root = It->second;
      bool Safe = true;
      for (Instruction *Between = I.getNextNode(); Between && Between != Root;
           Between = Between->getNextNode()) {
        if (isa<DbgInfoIntrinsic>(Between))
          continue;
        if (auto *II = dyn_cast<IntrinsicInst>(Between))
          if (II->getIntrinsicID() == Intrinsic::lifetime_start ||
              II->getIntrinsicID() == Intrinsic::lifetime_end)
            continue;
        auto BetweenRoot = Roots.find(Between);
        if (BetweenRoot == Roots.end() || BetweenRoot->second != Root) {
          Safe = false;
          break;
        }
      }
      if (Safe)
        State.Deferred.insert(&I);
    }
  }
}

bool ABCIRTranslator::emitBasicBlock(Function &F, BasicBlock &BB,
                                     FunctionState &State) {
  Streamer->emitLabel(getBlockSymbol(BB, State));
  for (Instruction &I : BB)
    if (!State.Deferred.contains(&I) && !emitInstruction(I, State))
      return false;
  return true;
}

bool ABCIRTranslator::emitFunction(Function &F) {
  if (F.isDeclaration())
    return true;

  FunctionState State;
  if (!buildFunctionState(F, State))
    return false;

  switchToText();
  MCSymbol *FnSym = getOrCreateSymbol(F.getName());
  if (!F.hasLocalLinkage())
    Streamer->emitSymbolAttribute(FnSym, MCSA_Global);
  Streamer->emitSymbolAttribute(FnSym, MCSA_ELF_TypeFunction);
  Streamer->emitLabel(FnSym);

  if (State.ReturnBytes && !emitPushZeroBytes(State.ReturnBytes))
    return false;

  if (State.LocalBytes)
    emitOpcodeImm(ABC::ALLOC, State.LocalBytes);

  if (F.getName() == "main") {
    auto *RetBytesSym = static_cast<MCSymbolELF *>(
        getOrCreateSymbol("$__abc.main.retbytes"));
    RetBytesSym->setType(ELF::STT_OBJECT);
    Streamer->emitAssignment(
        RetBytesSym, MCConstantExpr::create(State.ReturnBytes, *Ctx));
  }

  for (BasicBlock &BB : F)
    if (!emitBasicBlock(F, BB, State))
      return false;
  return true;
}

bool ABCIRTranslator::emitGlobals(Module &M) {
  for (GlobalVariable &GV : M.globals())
    if (!emitGlobal(GV))
      return false;
  return true;
}

bool ABCIRTranslator::emitGlobal(const GlobalVariable &GV) {
  if (GV.isDeclaration())
    return true;

  Type *ValueTy = GV.getValueType();
  unsigned Size = getTypeSize(ValueTy);
  if (!Size || Size > 255) {
    fail(&GV, "abc target only supports small globals in v1");
    return false;
  }

  MCSymbol *Sym = getOrCreateSymbol(GV.getName());
  if (!GV.hasLocalLinkage())
    Streamer->emitSymbolAttribute(Sym, MCSA_Global);
  Streamer->emitSymbolAttribute(Sym, MCSA_ELF_TypeObject);

  if (!GV.hasInitializer() || isZeroInitializer(GV.getInitializer())) {
    switchToBSS();
    Streamer->emitLabel(Sym);
    Streamer->emitZeros(Size);
    return true;
  }

  if (!GV.isConstant()) {
    fail(&GV, "abc target only supports zero-initialized mutable globals in v1");
    return false;
  }

  switchToROData();
  Streamer->emitLabel(Sym);
  if (auto *CDA = dyn_cast<ConstantDataArray>(GV.getInitializer())) {
    StringRef Bytes = CDA->getRawDataValues();
    Streamer->emitBytes(Bytes);
    return true;
  }
  if (auto *CI = dyn_cast<ConstantInt>(GV.getInitializer())) {
    Streamer->emitValue(MCConstantExpr::create(CI->getZExtValue(), *Ctx), Size);
    return true;
  }
  fail(&GV, "abc target does not support this constant global initializer");
  return false;
}

bool ABCIRTranslator::emitFunctions(Module &M) {
  for (Function &F : M)
    if (!emitFunction(F))
      return false;
  return true;
}

bool ABCIRTranslator::runOnModule(Module &M) {
  MOFI = Ctx->getObjectFileInfo();
  Streamer->initSections(TM.getMCSubtargetInfo());

  bool OK = emitGlobals(M) && emitFunctions(M);
  Streamer->finish();
  return OK && !Failed;
}
