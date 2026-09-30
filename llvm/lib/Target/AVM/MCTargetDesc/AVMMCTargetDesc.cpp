#include "AVMMCTargetDesc.h"
#include "AVMInstPrinter.h"
#include "AVMMCExpr.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCDisassembler/MCRelocationInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

void llvm::AVM_MC::canonicalizeMemoryInstruction(MCInst &Inst) {
  // Both parsed assembly and lowered machine instructions pass through here.
  // An explicit zero displacement need not pay for the displaced encoding.
  auto IsUpper = [](unsigned Reg) {
    return Reg == AVM::R4 || Reg == AVM::R5 || Reg == AVM::R6 ||
           Reg == AVM::R7;
  };
  auto IsLow = [](unsigned Reg) {
    return Reg == AVM::R0 || Reg == AVM::R1 || Reg == AVM::R2 ||
           Reg == AVM::R3;
  };

  unsigned Opcode = Inst.getOpcode();
  bool IsLoad = Opcode == AVM::DPLD8U || Opcode == AVM::DPLD16;
  bool IsStore = Opcode == AVM::DPST8 || Opcode == AVM::DPST16;
  if (IsLoad || IsStore) {
    unsigned AddrIndex = IsLoad ? 1 : 0;
    unsigned DispIndex = IsLoad ? 2 : 1;
    unsigned DataIndex = IsLoad ? 0 : 2;
    if (Inst.getNumOperands() != 3 || !Inst.getOperand(AddrIndex).isReg() ||
        !Inst.getOperand(DataIndex).isReg() ||
        !Inst.getOperand(DispIndex).isImm() ||
        Inst.getOperand(DispIndex).getImm() != 0 ||
        !IsUpper(Inst.getOperand(AddrIndex).getReg()))
      return;

    unsigned DataReg = Inst.getOperand(DataIndex).getReg();
    if (!IsUpper(DataReg) && !IsLow(DataReg))
      return;
    switch (Opcode) {
    case AVM::DPLD8U:
      Inst.setOpcode(IsUpper(DataReg) ? AVM::LD8U : AVM::F5LD8U);
      break;
    case AVM::DPLD16:
      Inst.setOpcode(IsUpper(DataReg) ? AVM::LD16 : AVM::F5LD16);
      break;
    case AVM::DPST8:
      Inst.setOpcode(IsUpper(DataReg) ? AVM::ST8 : AVM::F3ST8);
      break;
    case AVM::DPST16:
      Inst.setOpcode(IsUpper(DataReg) ? AVM::ST16 : AVM::F5ST16);
      break;
    }
    Inst.erase(Inst.begin() + DispIndex);
    return;
  }

  bool IsPostLoad = Opcode == AVM::GPLD8U_POST ||
                    Opcode == AVM::GPLD16_POST;
  bool IsPostStore = Opcode == AVM::GPST8_POST ||
                     Opcode == AVM::GPST16_POST;
  if ((IsPostLoad || IsPostStore) && Inst.getNumOperands() == 2 &&
      Inst.getOperand(IsPostLoad ? 1 : 0).isReg() &&
      IsUpper(Inst.getOperand(IsPostLoad ? 1 : 0).getReg())) {
    switch (Opcode) {
    case AVM::GPLD8U_POST:
      Inst.setOpcode(AVM::F7LD8U_POST);
      break;
    case AVM::GPLD16_POST:
      Inst.setOpcode(AVM::F7LD16_POST);
      break;
    case AVM::GPST8_POST:
      Inst.setOpcode(AVM::F6ST8_POST);
      break;
    case AVM::GPST16_POST:
      Inst.setOpcode(AVM::F7ST16_POST);
      break;
    }
  }
}

#define GET_INSTRINFO_MC_DESC
#define GET_INSTRINFO_MC_HELPERS
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "AVMGenInstrInfo.inc"
#define GET_REGINFO_MC_DESC
#include "AVMGenRegisterInfo.inc"
#define GET_SUBTARGETINFO_MC_DESC
#include "AVMGenSubtargetInfo.inc"

namespace {

class AVMObjectTargetStreamer final : public MCTargetStreamer {
public:
  explicit AVMObjectTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {
    static_cast<MCELFStreamer &>(S).getWriter().setELFHeaderEFlags(
        ELF::EF_AVM_ABI_V1);
  }
};

class AVMMCAsmInfo final : public MCAsmInfo {
public:
  AVMMCAsmInfo(const Triple &, const MCTargetOptions &) {
    CommentString = ";";
    SeparatorString = "\n";
    WeakRefDirective = "\t.weak\t";
    CodePointerSize = 3;
    CalleeSaveStackSlotSize = 2;
    MaxInstLength = 6;
    MinInstAlignment = 1;
    SupportsDebugInformation = false;
  }

  void printSpecifierExpr(raw_ostream &OS,
                          const MCSpecifierExpr &Expr) const override {
    StringRef Name;
    switch (Expr.getSpecifier()) {
    case AVM::VK_AVM_LO16: Name = "lo16"; break;
    case AVM::VK_AVM_HI8: Name = "hi8"; break;
    case AVM::VK_AVM_PROG24: Name = "prog24"; break;
    default: llvm_unreachable("unknown AVM expression modifier");
    }
    OS << '%' << Name << '(';
    printExpr(OS, *Expr.getSubExpr());
    OS << ')';
  }

  bool evaluateAsRelocatableImpl(const MCSpecifierExpr &Expr, MCValue &Res,
                                 const MCAssembler *Asm) const override {
    if (!Expr.getSubExpr()->evaluateAsRelocatable(Res, Asm))
      return false;
    if (!AVM::isAVMExprKind(Expr.getSpecifier()))
      return false;
    if (!Res.isAbsolute()) {
      Res.setSpecifier(Expr.getSpecifier());
      return !Res.getSubSym();
    }
    const int64_t Value = Res.getConstant();
    if (Value < 0 || Value > 0xffffff)
      return false;
    switch (Expr.getSpecifier()) {
    case AVM::VK_AVM_LO16: Res.setConstant(Value & 0xffff); break;
    case AVM::VK_AVM_HI8: Res.setConstant((Value >> 16) & 0xff); break;
    case AVM::VK_AVM_PROG24: break;
    default: llvm_unreachable("unknown AVM expression modifier");
    }
    return true;
  }
};

static MCAsmInfo *createAVMMCAsmInfo(const MCRegisterInfo &, const Triple &TT,
                                     const MCTargetOptions &Options) {
  return new AVMMCAsmInfo(TT, Options);
}

static MCInstrInfo *createAVMMCInstrInfo() {
  auto *Info = new MCInstrInfo();
  InitAVMMCInstrInfo(Info);
  return Info;
}

static MCRegisterInfo *createAVMMCRegisterInfo(const Triple &) {
  auto *Info = new MCRegisterInfo();
  InitAVMMCRegisterInfo(Info, 0);
  return Info;
}

static MCSubtargetInfo *createAVMMCSubtargetInfo(const Triple &TT,
                                                 StringRef CPU,
                                                 StringRef FS) {
  StringRef EffectiveCPU = CPU.empty() ? "avm1" : CPU;
  return createAVMMCSubtargetInfoImpl(TT, EffectiveCPU,
                                      "avm-interpreter-32u4-v1", FS);
}

static MCInstrAnalysis *createAVMMCInstrAnalysis(const MCInstrInfo *Info) {
  return new MCInstrAnalysis(Info);
}

static MCInstPrinter *createAVMMCInstPrinter(const Triple &, unsigned Variant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI) {
  if (Variant != 0)
    return nullptr;
  return new AVMInstPrinter(MAI, MII, MRI);
}

static MCTargetStreamer *createAVMObjectTargetStreamer(MCStreamer &S,
                                                        const MCSubtargetInfo &) {
  return new AVMObjectTargetStreamer(S);
}

static MCRelocationInfo *createAVMMCRelocationInfo(const Triple &TT,
                                                   MCContext &Ctx) {
  return llvm::createMCRelocationInfo(TT, Ctx);
}

} // namespace

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeAVMTargetMC() {
  Target &T = getTheAVMTarget();
  TargetRegistry::RegisterMCAsmInfo(T, createAVMMCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createAVMMCInstrInfo);
  TargetRegistry::RegisterMCInstrAnalysis(T, createAVMMCInstrAnalysis);
  TargetRegistry::RegisterMCRegInfo(T, createAVMMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createAVMMCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createAVMMCInstPrinter);
  TargetRegistry::RegisterMCCodeEmitter(T, createAVMMCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(T, createAVMAsmBackend);
  TargetRegistry::RegisterMCRelocationInfo(T, createAVMMCRelocationInfo);
  TargetRegistry::RegisterObjectTargetStreamer(T,
                                               createAVMObjectTargetStreamer);
}
