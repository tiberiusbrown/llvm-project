//===-- ABCAsmParser.cpp - Parse ABC assembly -----------------------------===//

#include "MCTargetDesc/ABCMCTargetDesc.h"
#include "MCTargetDesc/ABCInstrFormats.h"
#include "../ABCSyscalls.h"
#include "TargetInfo/ABCTargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

namespace {
class ABCOperand : public MCParsedAsmOperand {
  enum KindTy { Token, Imm } Kind;
  StringRef Tok;
  const MCExpr *Expr = nullptr;
  SMLoc Start;
  SMLoc End;

public:
  ABCOperand(StringRef Tok, SMLoc Loc)
      : Kind(Token), Tok(Tok), Start(Loc), End(Loc) {}
  ABCOperand(const MCExpr *Expr, SMLoc Start, SMLoc End)
      : Kind(Imm), Expr(Expr), Start(Start), End(End) {}

  bool isToken() const override { return Kind == Token; }
  bool isImm() const override { return Kind == Imm; }
  bool isReg() const override { return false; }
  MCRegister getReg() const override { return MCRegister(); }
  bool isMem() const override { return false; }
  SMLoc getStartLoc() const override { return Start; }
  SMLoc getEndLoc() const override { return End; }

  StringRef getToken() const { return Tok; }
  const MCExpr *getImm() const { return Expr; }

  void print(raw_ostream &OS, const MCAsmInfo &MAI) const override {
    if (Kind == Token)
      OS << Tok;
    else
      MCOperand::createExpr(Expr).print(OS);
  }
};

class ABCAsmParser : public MCTargetAsmParser {
  MCAsmParser &Parser;

public:
  ABCAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
               const MCInstrInfo &MII)
      : MCTargetAsmParser(STI, MII), Parser(Parser) {
    MCAsmParserExtension::Initialize(Parser);
  }

  bool matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;

  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                     SMLoc &EndLoc) override {
    return true;
  }

  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override {
    return ParseStatus::NoMatch;
  }

  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;

  ParseStatus parseDirective(AsmToken DirectiveID) override {
    return ParseStatus::NoMatch;
  }

  void convertToMapAndConstraints(unsigned Kind,
                                  const OperandVector &Operands) override {}

private:
  MCAsmParser &getParser() const { return Parser; }
  AsmLexer &getLexer() const { return Parser.getLexer(); }
};
} // namespace

bool ABCAsmParser::parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                    SMLoc NameLoc, OperandVector &Operands) {
  Operands.push_back(std::make_unique<ABCOperand>(Name, NameLoc));

  while (!getLexer().is(AsmToken::EndOfStatement)) {
    if (getLexer().is(AsmToken::Comma)) {
      getLexer().Lex();
      continue;
    }

    const MCExpr *Expr = nullptr;
    SMLoc Start = getLexer().getLoc();
    if (getParser().parseExpression(Expr))
      return true;
    Operands.push_back(
        std::make_unique<ABCOperand>(Expr, Start, getLexer().getLoc()));
  }
  return false;
}

static ABCOperand &getABCOperand(std::unique_ptr<MCParsedAsmOperand> &Op) {
  return static_cast<ABCOperand &>(*Op);
}

bool ABCAsmParser::matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                           OperandVector &Operands,
                                           MCStreamer &Out,
                                           uint64_t &ErrorInfo,
                                           bool MatchingInlineAsm) {
  StringRef Mnemonic = getABCOperand(Operands[0]).getToken();
  const ABC::InstrDesc *Desc = ABC::getInstrDescByMnemonic(Mnemonic);
  if (!Desc)
    return Error(IDLoc, "unknown ABC instruction");

  unsigned ExpectedOperands = Desc->NumOperands + 1;
  if (Operands.size() != ExpectedOperands)
    return Error(IDLoc, "wrong number of operands for ABC instruction");

  MCInst Inst;
  Inst.setOpcode(Desc->Opcode);
  for (unsigned I = 1; I < ExpectedOperands; ++I) {
    ABCOperand &Op = getABCOperand(Operands[I]);
    if (!Op.isImm())
      return Error(Op.getStartLoc(), "expected immediate or symbol operand");
    if (Desc->Opcode == ABC::SYS) {
      if (const auto *SRE = dyn_cast<MCSymbolRefExpr>(Op.getImm())) {
        auto Syscall = getABCSyscallImmediate(SRE->getSymbol().getName());
        if (!Syscall)
          return Error(Op.getStartLoc(), "unknown ABC syscall");
        Inst.addOperand(MCOperand::createImm(*Syscall));
        continue;
      }
    }
    if (const auto *CE = dyn_cast<MCConstantExpr>(Op.getImm()))
      Inst.addOperand(MCOperand::createImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(Op.getImm()));
  }

  Out.emitInstruction(Inst, getSTI());
  return false;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeABCAsmParser() {
  RegisterMCAsmParser<ABCAsmParser> X(getTheABCTarget());
}
