//===-- AVMAsmPrinter.cpp - AVM assembly and object emission --------------===//

#include "AVM.h"
#include "AVMMCInstLower.h"
#include "AVMTargetMachine.h"
#include "MCTargetDesc/AVMInstPrinter.h"
#include "MCTargetDesc/AVMMCExpr.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define DEBUG_TYPE "avm-asm-printer"

namespace {
class AVMAsmPrinter final : public AsmPrinter {
  static bool typeContainsProgramPointer(Type *Ty) {
    if (const auto *PtrTy = dyn_cast<PointerType>(Ty))
      return PtrTy->getAddressSpace() == 1;
    if (const auto *ArrayTy = dyn_cast<ArrayType>(Ty))
      return typeContainsProgramPointer(ArrayTy->getElementType());
    if (const auto *StructTy = dyn_cast<StructType>(Ty))
      return llvm::any_of(StructTy->elements(), typeContainsProgramPointer);
    return false;
  }

  static bool constantContainsProgramPointer(const Constant *C) {
    if (typeContainsProgramPointer(C->getType()))
      return true;
    for (const Use &Operand : C->operands())
      if (constantContainsProgramPointer(cast<Constant>(Operand)))
        return true;
    return false;
  }

  void emitAVMConstant(const DataLayout &DL, const Constant *C) {
    Type *Ty = C->getType();
    if (const auto *CE = dyn_cast<ConstantExpr>(C);
        CE && CE->getOpcode() == Instruction::PtrToInt &&
        Ty->isIntegerTy(24) &&
        CE->getOperand(0)->getType()->isPointerTy() &&
        CE->getOperand(0)->getType()->getPointerAddressSpace() == 1) {
      // Member-function pointers store a program address in an i24 field.
      emitAVMConstant(DL, cast<Constant>(CE->getOperand(0)));
      return;
    }
    if (const auto *PtrTy = dyn_cast<PointerType>(Ty);
        PtrTy && PtrTy->getAddressSpace() == 1) {
      if (isa<ConstantPointerNull, UndefValue>(C)) {
        OutStreamer->emitIntValue(0, 3);
        return;
      }
      if (const auto *CE = dyn_cast<ConstantExpr>(C)) {
        if (CE->getOpcode() == Instruction::AddrSpaceCast &&
            CE->getOperand(0)->getType()->getPointerAddressSpace() == 0) {
          // An RTTI data pointer occupies the low two bytes of the slot.
          OutStreamer->emitValue(lowerConstant(CE->getOperand(0)), 2);
          OutStreamer->emitIntValue(0, 1);
          return;
        }
        if (CE->getOpcode() == Instruction::IntToPtr)
          if (const auto *CI = dyn_cast<ConstantInt>(CE->getOperand(0))) {
            // Offset components use the low 24 bits in two's-complement form.
            OutStreamer->emitIntValue(CI->getZExtValue() & 0xffffff, 3);
            return;
          }
      }
      const MCExpr *Expr = MCSpecifierExpr::create(
          lowerConstant(C), AVM::VK_AVM_PROG24, OutContext);
      MCInst ProgPtr;
      ProgPtr.setOpcode(AVM::PROGPTR);
      ProgPtr.addOperand(MCOperand::createExpr(Expr));
      OutStreamer->emitInstruction(ProgPtr, *TM.getMCSubtargetInfo());
      return;
    }

    if (isa<ConstantAggregateZero, UndefValue>(C)) {
      OutStreamer->emitZeros(DL.getTypeAllocSize(Ty));
      return;
    }
    if (const auto *Array = dyn_cast<ConstantArray>(C)) {
      for (const Use &Element : Array->operands())
        emitAVMConstant(DL, cast<Constant>(Element));
      return;
    }
    if (const auto *Struct = dyn_cast<ConstantStruct>(C)) {
      const StructLayout *Layout = DL.getStructLayout(Struct->getType());
      uint64_t Offset = 0;
      for (unsigned I = 0; I != Struct->getNumOperands(); ++I) {
        uint64_t FieldOffset = Layout->getElementOffset(I);
        OutStreamer->emitZeros(FieldOffset - Offset);
        const Constant *Field = Struct->getOperand(I);
        emitAVMConstant(DL, Field);
        Offset = FieldOffset + DL.getTypeAllocSize(Field->getType());
      }
      OutStreamer->emitZeros(Layout->getSizeInBytes() - Offset);
      return;
    }
    emitGlobalConstant(DL, C);
  }

public:
  AVMAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  void emitXXStructor(const DataLayout &DL, const Constant *CV) override {
    // Function pointers in init/fini arrays use the same packed program
    // address and R_AVM_PROG24 relocation as other AS1 initializers.
    emitAVMConstant(DL, CV);
  }

  StringRef getPassName() const override { return "AVM Assembly Printer"; }

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &OS) override {
    if (ExtraCode && ExtraCode[0])
      return true;
    const MachineOperand &MO = MI->getOperand(OpNo);
    if (MO.isReg()) {
      OS << AVMInstPrinter::getRegisterName(MO.getReg());
      return false;
    }
    if (MO.isImm()) {
      OS << MO.getImm();
      return false;
    }
    return AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, OS);
  }

  bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo,
                             const char *ExtraCode, raw_ostream &OS) override {
    if (ExtraCode && ExtraCode[0])
      return true;
    OS << '[';
    if (PrintAsmOperand(MI, OpNo, nullptr, OS))
      return true;
    OS << ']';
    return false;
  }

  void emitGlobalVariable(const GlobalVariable *GV) override {
    if (!GV->hasInitializer() ||
        !constantContainsProgramPointer(GV->getInitializer())) {
      AsmPrinter::emitGlobalVariable(GV);
      return;
    }
    if (emitSpecialLLVMGlobal(GV))
      return;

    MCSymbol *Symbol = getSymbol(GV);
    emitVisibility(Symbol, GV->getVisibility(), true);
    Symbol->redefineIfPossible();
    if (MAI->hasDotTypeDotSizeDirective())
      OutStreamer->emitSymbolAttribute(Symbol, MCSA_ELF_TypeObject);

    SectionKind Kind = TargetLoweringObjectFile::getKindForGlobal(GV, TM);
    MCSection *Section = getObjFileLowering().SectionForGlobal(GV, Kind, TM);
    const DataLayout &DL = GV->getDataLayout();
    uint64_t Size = DL.getTypeAllocSize(GV->getValueType());
    OutStreamer->switchSection(Section);
    emitLinkage(GV, Symbol);
    emitAlignment(getGVAlignment(GV, DL), GV);
    OutStreamer->emitLabel(Symbol);
    MCSymbol *LocalAlias = getSymbolPreferLocal(*GV);
    if (LocalAlias != Symbol)
      OutStreamer->emitLabel(LocalAlias);
    emitAVMConstant(DL, GV->getInitializer());
    if (MAI->hasDotTypeDotSizeDirective())
      OutStreamer->emitELFSize(Symbol,
                               MCConstantExpr::create(Size, OutContext));
    OutStreamer->addBlankLine();
  }

  void emitInstruction(const MachineInstr *MI) override {
    AVM_MC::verifyInstructionPredicates(MI->getOpcode(),
                                        getSubtargetInfo().getFeatureBits());
    AVMMCInstLower Lowering(OutContext, *this);
    MCInst OutMI;
    Lowering.lower(MI, OutMI);
    AVM_MC::canonicalizeMemoryInstruction(OutMI);
    EmitToStreamer(*OutStreamer, OutMI);
  }

  static char ID;
};
} // namespace

char AVMAsmPrinter::ID = 0;

INITIALIZE_PASS(AVMAsmPrinter, DEBUG_TYPE, "AVM Assembly Printer", false, false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVMAsmPrinter() {
  RegisterAsmPrinter<AVMAsmPrinter> X(getTheAVMTarget());
}
