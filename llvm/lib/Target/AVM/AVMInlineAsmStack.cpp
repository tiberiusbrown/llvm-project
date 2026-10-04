//===-- AVMInlineAsmStack.cpp - Inline assembly stack certification
//--------===//

#include "AVMInlineAsmStack.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

namespace {
class StackCheckingStreamer final : public MCStreamer {
  const MCInstrInfo &MII;
  DenseSet<const MCSymbol *> Labels;
  SmallVector<const MCSymbol *, 4> BranchTargets;
  bool Safe = true;

public:
  StackCheckingStreamer(MCContext &Context, const MCInstrInfo &MII)
      : MCStreamer(Context), MII(MII) {}

  bool emitSymbolAttribute(MCSymbol *, MCSymbolAttr) override {
    Safe = false;
    return false;
  }
  void emitCommonSymbol(MCSymbol *, uint64_t, Align) override { Safe = false; }
  void emitBytes(StringRef) override { Safe = false; }
  void emitValueImpl(const MCExpr *, unsigned, SMLoc) override { Safe = false; }
  void emitIntValue(uint64_t, unsigned) override { Safe = false; }
  void emitLabel(MCSymbol *Symbol, SMLoc Loc) override {
    Labels.insert(Symbol);
    MCStreamer::emitLabel(Symbol, Loc);
  }

  void emitInstruction(const MCInst &Inst, const MCSubtargetInfo &) override {
    const MCInstrDesc &Desc = MII.get(Inst.getOpcode());
    if (Desc.isCall() || Desc.isReturn() || Desc.isIndirectBranch() ||
        Desc.hasImplicitDefOfPhysReg(AVM::SP) ||
        Desc.hasImplicitDefOfPhysReg(AVM::PC) ||
        Inst.getOpcode() == AVM::PROGPTR || !Desc.getSize()) {
      Safe = false;
      return;
    }
    for (unsigned I = 0; I != Desc.getNumDefs(); ++I)
      if (Inst.getOperand(I).isReg() &&
          (Inst.getOperand(I).getReg() == AVM::SP ||
           Inst.getOperand(I).getReg() == AVM::PC))
        Safe = false;

    if (Inst.getOpcode() == AVM::SYS) {
      // Only architectural services with defined interfaces are supported.
      switch (Inst.getOperand(0).getImm()) {
#define AVM_SYS_DEF(ID, ...)                                                   \
  case ID:                                                                     \
    break;
#include "AVMSystemCalls.inc"
      default:
        Safe = false;
        break;
      }
    }
    if (!Desc.isBranch())
      return;
    // A jump does not change SP, but it may enter arbitrary stack-using code.
    // Accept only exact labels emitted within this particular assembly block.
    if (Inst.getNumOperands() != 1 || !Inst.getOperand(0).isExpr()) {
      Safe = false;
      return;
    }
    const auto *Target =
        dyn_cast<MCSymbolRefExpr>(Inst.getOperand(0).getExpr());
    if (!Target) {
      Safe = false;
      return;
    }
    BranchTargets.push_back(&Target->getSymbol());
  }

  bool isSafe() const {
    return Safe && llvm::all_of(BranchTargets, [&](const MCSymbol *Target) {
             return Labels.contains(Target);
           });
  }
};

bool hasOnlyInstructionsAndLabels(StringRef Assembly, const MCAsmInfo &MAI,
                                  MCContext &Original) {
  // Directives can emit instructions as raw bytes, include files, switch
  // sections, or define macros/aliases. Do not execute them in the checker.
  // Use the MC lexer so comments and dotted local labels are handled correctly.
  AsmLexer Lexer(MAI);
  Lexer.setBuffer(Assembly);
  bool AtStatementStart = true;
  for (AsmToken Tok = Lexer.Lex(); Tok.isNot(AsmToken::Eof);
       Tok = Lexer.Lex()) {
    if (Tok.is(AsmToken::Error) || Tok.is(AsmToken::Equal))
      return false;
    if (Tok.is(AsmToken::EndOfStatement)) {
      AtStatementStart = true;
      continue;
    }
    if (AtStatementStart && Lexer.peekTok().isNot(AsmToken::Colon)) {
      // A macro from module assembly or an earlier block can even shadow an
      // instruction mnemonic. Never certify its isolated interpretation.
      // Quoted identifiers can name directives too; only plain mnemonics pass.
      if (!Tok.is(AsmToken::Identifier) ||
          Tok.getIdentifier().starts_with(".") ||
          Original.lookupMacro(Tok.getIdentifier()))
        return false;
    }
    // A label can precede an instruction or directive on the same line.
    AtStatementStart = Tok.is(AsmToken::Colon);
  }
  return true;
}
} // namespace

bool llvm::isAVMInlineAsmStackSafe(StringRef Assembly, const TargetMachine &TM,
                                   const MCSubtargetInfo &STI,
                                   MCContext &Original) {
  if (!hasOnlyInstructionsAndLabels(Assembly, *TM.getMCAsmInfo(), Original))
    return false;

  SourceMgr Sources;
  Sources.AddNewSourceBuffer(MemoryBuffer::getMemBufferCopy(Assembly), SMLoc());
  // The real emission reports malformed assembly. A failed certification must
  // not issue duplicate diagnostics or mutate the real assembler's symbols.
  Sources.setDiagHandler([](const SMDiagnostic &, void *) {});
  MCContext Context(TM.getTargetTriple(), TM.getMCAsmInfo(),
                    TM.getMCRegisterInfo(), &STI, &Sources,
                    &TM.Options.MCOptions);
  MCObjectFileInfo ObjectInfo;
  ObjectInfo.initMCObjectFileInfo(Context, /*PIC=*/false);
  Context.setObjectFileInfo(&ObjectInfo);
  std::unique_ptr<MCInstrInfo> MII(TM.getTarget().createMCInstrInfo());
  StackCheckingStreamer Streamer(Context, *MII);
  Streamer.initSections(/*NoExecStack=*/false, STI);
  std::unique_ptr<MCAsmParser> Parser(
      createMCAsmParser(Sources, Context, Streamer, *TM.getMCAsmInfo()));
  std::unique_ptr<MCTargetAsmParser> TargetParser(
      TM.getTarget().createMCAsmParser(STI, *Parser, *MII,
                                       TM.Options.MCOptions));
  if (!TargetParser)
    return false;
  Parser->setTargetParser(*TargetParser);
  return !Parser->Run(/*NoInitialTextSection=*/true, /*NoFinalize=*/true) &&
         Streamer.isSafe();
}
