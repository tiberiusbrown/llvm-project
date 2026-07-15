#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include <memory>
#include <optional>
#include <string>

using namespace llvm;

namespace {

class AVMParsedOperand final : public MCParsedAsmOperand {
  std::string Text;
  SMLoc Start;
  SMLoc End;

public:
  AVMParsedOperand(StringRef Text, SMLoc Start, SMLoc End)
      : Text(Text.str()), Start(Start), End(End) {}

  bool isToken() const override { return true; }
  bool isImm() const override { return false; }
  bool isReg() const override { return false; }
  MCRegister getReg() const override { return MCRegister(); }
  bool isMem() const override { return false; }
  SMLoc getStartLoc() const override { return Start; }
  SMLoc getEndLoc() const override { return End; }
  void print(raw_ostream &OS, const MCAsmInfo &) const override {
    OS << "Token: " << Text;
  }
};

class AVMAsmParser final : public MCTargetAsmParser {
  MCAsmParser &Parser;
  std::optional<MCInst> Pending;

  bool error(SMLoc Loc, const Twine &Message) {
    return Parser.Error(Loc, Message);
  }

  bool parseArchitecturalReg(MCRegister &Reg, SMLoc *Start = nullptr,
                             SMLoc *End = nullptr) {
    const AsmToken &Tok = Parser.getTok();
    if (!Tok.is(AsmToken::Identifier))
      return error(Tok.getLoc(), "expected AVM register");

    Reg = StringSwitch<MCRegister>(Tok.getIdentifier().lower())
              .Case("r0", AVM::R0).Case("r1", AVM::R1)
              .Case("r2", AVM::R2).Case("r3", AVM::R3)
              .Case("r4", AVM::R4).Case("r5", AVM::R5)
              .Case("r6", AVM::R6).Case("r7", AVM::R7)
              .Case("c0", AVM::R4).Case("c1", AVM::R5)
              .Case("c2", AVM::R6).Case("c3", AVM::R7)
              .Case("q0", AVM::R0R1).Case("q1", AVM::R2R3)
              .Case("q2", AVM::R4R5).Case("q3", AVM::R6R7)
              .Case("sp", AVM::SP).Case("pc", AVM::PC).Case("cc", AVM::CC)
              .Default(MCRegister());
    if (!Reg)
      return error(Tok.getLoc(), "unknown AVM register");
    if (Start)
      *Start = Tok.getLoc();
    if (End)
      *End = Tok.getEndLoc();
    Parser.Lex();
    return false;
  }

  static void addExpr(MCInst &Inst, const MCExpr *Expr) {
    if (const auto *Constant = dyn_cast<MCConstantExpr>(Expr))
      Inst.addOperand(MCOperand::createImm(Constant->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
  }

  bool finishInstruction(MCInst &&Inst, SMLoc EndLoc,
                         OperandVector &Operands, StringRef Name,
                         SMLoc NameLoc) {
    if (Parser.parseEOL("unexpected token after AVM instruction"))
      return true;
    Inst.setLoc(NameLoc);
    Pending = std::move(Inst);
    Operands.push_back(
        std::make_unique<AVMParsedOperand>(Name, NameLoc, EndLoc));
    return false;
  }

  bool parseFarTransfer(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands) {
    const MCExpr *Expr = nullptr;
    if (Parser.parseExpression(Expr))
      return true;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    addExpr(Inst, Expr);
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

public:
  AVMAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
               const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII), Parser(Parser) {}

  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                     SMLoc &EndLoc) override {
    return parseArchitecturalReg(Reg, &StartLoc, &EndLoc);
  }

  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override {
    if (!Parser.getTok().is(AsmToken::Identifier))
      return ParseStatus::NoMatch;
    StringRef Name = Parser.getTok().getIdentifier();
    if (!Name.starts_with_insensitive("r") &&
        !Name.starts_with_insensitive("c") &&
        !Name.starts_with_insensitive("q") &&
        !Name.equals_insensitive("sp") && !Name.equals_insensitive("pc") &&
        !Name.equals_insensitive("cc"))
      return ParseStatus::NoMatch;
    if (parseArchitecturalReg(Reg, &StartLoc, &EndLoc))
      return ParseStatus::Failure;
    return ParseStatus::Success;
  }

  void convertToMapAndConstraints(unsigned, const OperandVector &) override {}

  ParseStatus parseDirective(AsmToken) override {
    return ParseStatus::NoMatch;
  }

  bool parseInstruction(ParseInstructionInfo &, StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands) override {
    Pending.reset();
    std::string Lower = Name.lower();
    if (Lower == "jmpf")
      return parseFarTransfer(AVM::JMPF, Name, NameLoc, Operands);
    if (Lower == "callf")
      return parseFarTransfer(AVM::CALLF, Name, NameLoc, Operands);
    return error(NameLoc, Twine("unknown AVM instruction '") + Name + "'");
  }

  bool matchAndEmitInstruction(SMLoc Loc, unsigned &, OperandVector &,
                               MCStreamer &Out, uint64_t &, bool) override {
    if (!Pending)
      return error(Loc, "internal AVM parser error: no pending instruction");
    Out.emitInstruction(*Pending, getSTI());
    Pending.reset();
    return false;
  }
};

} // namespace

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeAVMAsmParser() {
  RegisterMCAsmParser<AVMAsmParser> X(getTheAVMTarget());
}
