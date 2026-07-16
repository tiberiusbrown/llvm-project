#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "MCTargetDesc/AVMMCExpr.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSectionELF.h"
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

  bool parseProgramAddressExpr(const MCExpr *&Expr, SMLoc &ExprLoc) {
    ExprLoc = Parser.getTok().getLoc();
    if (Parser.parseExpression(Expr))
      return true;
    int64_t Value = 0;
    if (Expr->evaluateAsAbsolute(Value) && (Value < 0 || Value > 0xffffff))
      return error(ExprLoc, "program address is out of unsigned 24-bit range");
    return false;
  }

  bool parseLoadableImmediate(unsigned Bits, const MCExpr *&Expr,
                              SMLoc &ExprLoc) {
    ExprLoc = Parser.getTok().getLoc();
    if (Parser.parseExpression(Expr))
      return true;
    int64_t Value = 0;
    if (Expr->evaluateAsAbsolute(Value)) {
      if (Value < 0 || Value >= (int64_t(1) << Bits))
        return error(ExprLoc, "immediate is out of range");
      return false;
    }
    const auto *Spec = dyn_cast<MCSpecifierExpr>(Expr);
    const unsigned Required = Bits == 16 ? AVM::VK_AVM_LO16 : AVM::VK_AVM_HI8;
    if (Spec && Spec->getSpecifier() != Required)
      return error(ExprLoc, "incompatible AVM expression modifier for immediate width");
    if (Bits == 8 && !Spec)
      return error(ExprLoc, "immediate expression must be fully resolvable");
    return false;
  }

  static bool isArchitecturalRegisterIdentifier(StringRef Identifier) {
    return StringSwitch<bool>(Identifier.lower())
        .Case("r0", true).Case("r1", true).Case("r2", true).Case("r3", true)
        .Case("r4", true).Case("r5", true).Case("r6", true).Case("r7", true)
        .Case("c0", true).Case("c1", true).Case("c2", true).Case("c3", true)
        .Case("q0", true).Case("q1", true).Case("q2", true).Case("q3", true)
        .Case("sp", true).Case("pc", true).Case("cc", true)
        .Default(false);
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

  bool parseRel16Control(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                         OperandVector &Operands) {
    SMLoc ExprLoc = Parser.getTok().getLoc();
    const MCExpr *Expr = nullptr;
    if (Parser.parseExpression(Expr))
      return true;
    int64_t Value = 0;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    if (Expr->evaluateAsAbsolute(Value)) {
      if (Value < -32768 || Value > 32767)
        return error(ExprLoc, "relative displacement is out of signed 16-bit range");
      Inst.addOperand(MCOperand::createImm(Value));
    } else {
      Inst.addOperand(MCOperand::createExpr(Expr));
    }
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseProgramPair(MCRegister &Reg) {
    const AsmToken &Tok = Parser.getTok();
    if (!Tok.is(AsmToken::Identifier))
      return error(Tok.getLoc(), "expected program pair q0-q3");
    Reg = StringSwitch<MCRegister>(Tok.getIdentifier().lower())
              .Case("q0", AVM::R0R1).Case("q1", AVM::R2R3)
              .Case("q2", AVM::R4R5).Case("q3", AVM::R6R7)
              .Default(MCRegister());
    if (!Reg)
      return error(Tok.getLoc(), "expected program pair q0-q3");
    Parser.Lex();
    return false;
  }

  bool parseProgramPairTransfer(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                                OperandVector &Operands) {
    MCRegister Reg;
    if (parseProgramPair(Reg))
      return true;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Reg));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseOperandless(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands) {
    MCInst Inst;
    Inst.setOpcode(Opcode);
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseSignedImmediate(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                            OperandVector &Operands) {
    SMLoc ExprLoc = Parser.getTok().getLoc();
    const MCExpr *Expr = nullptr;
    if (Parser.parseExpression(Expr))
      return true;
    int64_t Value = 0;
    if (!Expr->evaluateAsAbsolute(Value))
      return error(ExprLoc, "immediate expression must be fully resolvable");
    if (Value < -128 || Value > 127)
      return error(ExprLoc, "immediate is out of signed 8-bit range");
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createImm(Value));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseRel8Control(unsigned Opcode, bool AllowSymbol, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) {
    SMLoc ExprLoc = Parser.getTok().getLoc();
    if (Parser.getTok().is(AsmToken::Identifier) &&
        isArchitecturalRegisterIdentifier(Parser.getTok().getIdentifier()))
      return error(ExprLoc, "expected relative displacement expression");
    const MCExpr *Expr = nullptr;
    if (Parser.parseExpression(Expr))
      return true;
    int64_t Value = 0;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    if (Expr->evaluateAsAbsolute(Value)) {
      if (Value < -128 || Value > 127)
        return error(ExprLoc, "relative displacement is out of signed 8-bit range");
      Inst.addOperand(MCOperand::createImm(Value));
    } else if (AllowSymbol) {
      Inst.addOperand(MCOperand::createExpr(Expr));
    } else {
      return error(ExprLoc, "symbolic call is deferred to relaxable pseudo support");
    }
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseRelaxableTransfer(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                              OperandVector &Operands) {
    SMLoc ExprLoc = Parser.getTok().getLoc();
    if (Parser.getTok().is(AsmToken::Identifier)) {
      if (isArchitecturalRegisterIdentifier(Parser.getTok().getIdentifier()))
        return error(ExprLoc, "expected relocatable symbolic program target");
    }
    const MCExpr *Expr = nullptr;
    if (Parser.parseExpression(Expr))
      return true;
    int64_t Value = 0;
    if (Expr->evaluateAsAbsolute(Value))
      return error(ExprLoc, Twine("relaxable '") + Name +
                                "' requires a symbolic target; use the exact control-transfer mnemonic for constants");
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createExpr(Expr));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseService(StringRef Name, SMLoc NameLoc, OperandVector &Operands) {
    SMLoc ExprLoc = Parser.getTok().getLoc();
    const MCExpr *Expr = nullptr;
    if (Parser.parseExpression(Expr))
      return true;
    int64_t Value = 0;
    if (!Expr->evaluateAsAbsolute(Value))
      return error(ExprLoc, "service expression must be fully resolvable");
    if (Value < 0 || Value > 3)
      return error(ExprLoc, "invalid AVM version 1 service identifier");
    MCInst Inst;
    Inst.setOpcode(AVM::SYS);
    Inst.addOperand(MCOperand::createImm(Value));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseCompactReg(MCRegister &Reg) {
    SMLoc Loc = Parser.getTok().getLoc();
    if (Parser.getTok().is(AsmToken::Identifier) &&
        !Parser.getTok().getIdentifier().starts_with_insensitive("c"))
      return error(Loc, "expected compact register c0-c3");
    if (parseArchitecturalReg(Reg))
      return true;
    switch (Reg.id()) {
    case AVM::R4:
    case AVM::R5:
    case AVM::R6:
    case AVM::R7:
      return false;
    default:
      return error(Loc, "expected compact register c0-c3");
    }
  }

  bool parseStackReg(MCRegister &Reg) {
    const AsmToken &Tok = Parser.getTok();
    if (!Tok.is(AsmToken::Identifier))
      return error(Tok.getLoc(), "expected full register r0-r7");
    Reg = StringSwitch<MCRegister>(Tok.getIdentifier().lower())
              .Case("r0", AVM::R0).Case("r1", AVM::R1)
              .Case("r2", AVM::R2).Case("r3", AVM::R3)
              .Case("r4", AVM::R4).Case("r5", AVM::R5)
              .Case("r6", AVM::R6).Case("r7", AVM::R7)
              .Default(MCRegister());
    if (!Reg)
      return error(Tok.getLoc(), "expected full register r0-r7");
    Parser.Lex();
    return false;
  }

  bool parseColdReg(MCRegister &Reg) {
    const AsmToken &Tok = Parser.getTok();
    if (!Tok.is(AsmToken::Identifier))
      return error(Tok.getLoc(), "expected cold register r0-r3");
    Reg = StringSwitch<MCRegister>(Tok.getIdentifier().lower())
              .Case("r0", AVM::R0).Case("r1", AVM::R1)
              .Case("r2", AVM::R2).Case("r3", AVM::R3)
              .Default(MCRegister());
    if (!Reg)
      return error(Tok.getLoc(), "expected cold register r0-r3");
    Parser.Lex();
    return false;
  }

  bool parseAbsoluteDataReg(MCRegister &Reg) {
    const AsmToken &Tok = Parser.getTok();
    if (!Tok.is(AsmToken::Identifier))
      return error(Tok.getLoc(), "expected full register r0-r7");
    Reg = StringSwitch<MCRegister>(Tok.getIdentifier().lower())
              .Case("r0", AVM::R0).Case("r1", AVM::R1)
              .Case("r2", AVM::R2).Case("r3", AVM::R3)
              .Case("r4", AVM::R4).Case("r5", AVM::R5)
              .Case("r6", AVM::R6).Case("r7", AVM::R7)
              .Default(MCRegister());
    if (!Reg)
      return error(Tok.getLoc(), "expected full register r0-r7");
    Parser.Lex();
    return false;
  }

  bool parseAbsoluteDataAddress(const MCExpr *&Expr, SMLoc &ExprLoc) {
    if (!Parser.getTok().is(AsmToken::LBrac))
      return error(Parser.getTok().getLoc(),
                   "expected absolute memory operand '[addr16]'");
    Parser.Lex();
    ExprLoc = Parser.getTok().getLoc();
    if (Parser.parseExpression(Expr))
      return true;
    int64_t Value = 0;
    if (Expr->evaluateAsAbsolute(Value) && (Value < 0 || Value > 65535))
      return error(ExprLoc, "absolute address is out of unsigned 16-bit range");
    if (!Parser.getTok().is(AsmToken::RBrac))
      return error(Parser.getTok().getLoc(),
                   "expected ']' after absolute address");
    Parser.Lex();
    return false;
  }

  bool parseAbsoluteDataInstruction(unsigned Opcode, bool IsStore,
                                    StringRef Name, SMLoc NameLoc,
                                    OperandVector &Operands) {
    MCRegister Reg;
    const MCExpr *Expr = nullptr;
    SMLoc ExprLoc;
    if (IsStore) {
      if (parseAbsoluteDataAddress(Expr, ExprLoc) || Parser.parseComma() ||
          parseAbsoluteDataReg(Reg))
        return true;
    } else {
      if (parseAbsoluteDataReg(Reg) || Parser.parseComma() ||
          parseAbsoluteDataAddress(Expr, ExprLoc))
        return true;
    }
    MCInst Inst;
    Inst.setOpcode(Opcode);
    if (IsStore)
      addExpr(Inst, Expr);
    Inst.addOperand(MCOperand::createReg(Reg));
    if (!IsStore)
      addExpr(Inst, Expr);
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseAbsoluteU8(uint64_t &Value, SMLoc &ExprLoc) {
    ExprLoc = Parser.getTok().getLoc();
    const MCExpr *Expr = nullptr;
    if (Parser.parseExpression(Expr))
      return true;
    int64_t SignedValue = 0;
    if (!Expr->evaluateAsAbsolute(SignedValue))
      return error(ExprLoc, "immediate expression must be fully resolvable");
    if (SignedValue < 0 || SignedValue > 255)
      return error(ExprLoc, "immediate is out of unsigned 8-bit range");
    Value = SignedValue;
    return false;
  }

  bool parseColdImmediate(unsigned Opcode, bool IsSigned, unsigned Bits,
                          StringRef Name, SMLoc NameLoc,
                          OperandVector &Operands) {
    MCRegister Reg;
    if (parseColdReg(Reg) || Parser.parseComma())
      return true;
    const MCExpr *Expr = nullptr;
    SMLoc ExprLoc;
    if (!IsSigned && (Bits == 8 || Bits == 16)) {
      if (parseLoadableImmediate(Bits, Expr, ExprLoc))
        return true;
      MCInst Inst;
      Inst.setOpcode(Opcode);
      Inst.addOperand(MCOperand::createReg(Reg));
      addExpr(Inst, Expr);
      return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                               Operands, Name, NameLoc);
    } else if (Parser.parseExpression(Expr)) return true;
    int64_t Value = 0;
    if (!Expr->evaluateAsAbsolute(Value))
      return error(ExprLoc, "immediate expression must be fully resolvable");
    const int64_t Min = IsSigned ? -(int64_t(1) << (Bits - 1)) : 0;
    const int64_t Max = IsSigned ? (int64_t(1) << (Bits - 1)) - 1
                                 : (int64_t(1) << Bits) - 1;
    if (Value < Min || Value > Max)
      return error(ExprLoc, "immediate is out of range");
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Reg));
    Inst.addOperand(MCOperand::createImm(Value));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseSPMemory(unsigned &Offset) {
    if (!Parser.getTok().is(AsmToken::LBrac))
      return error(Parser.getTok().getLoc(), "expected stack memory operand '[sp+u8]'");
    Parser.Lex();
    const AsmToken &Tok = Parser.getTok();
    if (!Tok.is(AsmToken::Identifier) || !Tok.getIdentifier().equals_insensitive("sp"))
      return error(Tok.getLoc(), "expected stack pointer 'sp'");
    Parser.Lex();
    if (!Parser.getTok().is(AsmToken::Plus))
      return error(Parser.getTok().getLoc(), "expected '+' and stack offset");
    Parser.Lex();
    uint64_t Value = 0;
    SMLoc ExprLoc;
    if (parseAbsoluteU8(Value, ExprLoc))
      return true;
    Offset = static_cast<unsigned>(Value);
    if (!Parser.getTok().is(AsmToken::RBrac))
      return error(Parser.getTok().getLoc(), "expected ']' after stack offset");
    Parser.Lex();
    return false;
  }

  bool parseSTSP8(StringRef Name, SMLoc NameLoc, OperandVector &Operands) {
    unsigned Offset = 0;
    MCRegister Reg;
    if (parseSPMemory(Offset) || Parser.parseComma())
      return true;
    MCInst Inst;
    const AsmToken &Tok = Parser.getTok();
    const bool IsCompact = Tok.is(AsmToken::Identifier) &&
                           Tok.getIdentifier().starts_with_insensitive("c");
    if (IsCompact) {
      if (Offset > 15)
        return error(Tok.getLoc(), "compact stack offset is out of unsigned 4-bit range");
      if (parseCompactReg(Reg))
        return true;
      Inst.setOpcode(AVM::STSP8_COMPACT);
    } else {
      if (parseStackReg(Reg))
        return true;
      Inst.setOpcode(AVM::STSP8);
    }
    Inst.addOperand(MCOperand::createImm(Offset));
    Inst.addOperand(MCOperand::createReg(Reg));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseLEASP(StringRef Name, SMLoc NameLoc, OperandVector &Operands) {
    MCRegister Reg;
    if (parseStackReg(Reg) || Parser.parseComma())
      return true;
    uint64_t Value = 0;
    SMLoc ExprLoc;
    if (parseAbsoluteU8(Value, ExprLoc))
      return true;
    MCInst Inst;
    Inst.setOpcode(AVM::LEASP);
    Inst.addOperand(MCOperand::createReg(Reg));
    Inst.addOperand(MCOperand::createImm(Value));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(), Operands,
                             Name, NameLoc);
  }

  bool parseSPMemoryInstruction(unsigned Opcode, bool IsStore, StringRef Name,
                                SMLoc NameLoc, OperandVector &Operands) {
    MCRegister Reg;
    unsigned Offset = 0;
    if (IsStore) {
      if (parseSPMemory(Offset) || Parser.parseComma() || parseStackReg(Reg))
        return true;
    } else {
      if (parseStackReg(Reg) || Parser.parseComma() || parseSPMemory(Offset))
        return true;
    }
    MCInst Inst;
    Inst.setOpcode(Opcode);
    if (IsStore)
      Inst.addOperand(MCOperand::createImm(Offset));
    Inst.addOperand(MCOperand::createReg(Reg));
    if (!IsStore)
      Inst.addOperand(MCOperand::createImm(Offset));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(), Operands,
                             Name, NameLoc);
  }

  bool parseLDSP8U(StringRef Name, SMLoc NameLoc,
                   OperandVector &Operands) {
    const bool IsCompact = Parser.getTok().is(AsmToken::Identifier) &&
                           Parser.getTok().getIdentifier().starts_with_insensitive("c");
    MCRegister Reg;
    unsigned Offset = 0;
    if (IsCompact) {
      if (parseCompactReg(Reg) || Parser.parseComma() || parseSPMemory(Offset))
        return true;
      if (Offset > 15)
        return error(Parser.getTok().getLoc(),
                     "compact stack offset is out of unsigned 4-bit range");
    } else {
      if (parseStackReg(Reg) || Parser.parseComma() || parseSPMemory(Offset))
        return true;
    }
    MCInst Inst;
    Inst.setOpcode(IsCompact ? AVM::LDSP8U_COMPACT : AVM::LDSP8U);
    Inst.addOperand(MCOperand::createReg(Reg));
    Inst.addOperand(MCOperand::createImm(Offset));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseSPWordInstruction(StringRef Name, SMLoc NameLoc,
                              OperandVector &Operands, bool IsStore) {
    MCRegister Reg;
    unsigned Offset = 0;
    if (IsStore) {
      if (parseSPMemory(Offset) || Parser.parseComma())
        return true;
      const AsmToken &Tok = Parser.getTok();
      const bool IsCompact = Tok.is(AsmToken::Identifier) &&
                             Tok.getIdentifier().starts_with_insensitive("c");
      if (IsCompact) {
        if (Offset > 15)
          return error(Tok.getLoc(),
                       "compact stack offset is out of unsigned 4-bit range");
        if (parseCompactReg(Reg))
          return true;
      } else if (parseStackReg(Reg)) {
        return true;
      }
      MCInst Inst;
      Inst.setOpcode(IsCompact ? AVM::STSP16_COMPACT : AVM::STSP16);
      Inst.addOperand(MCOperand::createImm(Offset));
      Inst.addOperand(MCOperand::createReg(Reg));
      return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                               Operands, Name, NameLoc);
    }

    const AsmToken &Tok = Parser.getTok();
    const bool IsCompact = Tok.is(AsmToken::Identifier) &&
                           Tok.getIdentifier().starts_with_insensitive("c");
    if (IsCompact) {
      if (parseCompactReg(Reg) || Parser.parseComma() || parseSPMemory(Offset))
        return true;
      if (Offset > 15)
        return error(Tok.getLoc(),
                     "compact stack offset is out of unsigned 4-bit range");
    } else if (parseStackReg(Reg) || Parser.parseComma() ||
               parseSPMemory(Offset)) {
      return true;
    }
    MCInst Inst;
    Inst.setOpcode(IsCompact ? AVM::LDSP16_COMPACT : AVM::LDSP16);
    Inst.addOperand(MCOperand::createReg(Reg));
    Inst.addOperand(MCOperand::createImm(Offset));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseStackInstruction(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                             OperandVector &Operands) {
    MCRegister Reg;
    if (parseStackReg(Reg))
      return true;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Reg));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseCompactMemory(MCRegister &Reg) {
    if (!Parser.getTok().is(AsmToken::LBrac))
      return error(Parser.getTok().getLoc(), "expected compact memory operand '[cN]'");
    Parser.Lex();
    if (parseCompactReg(Reg))
      return true;
    if (Parser.getTok().is(AsmToken::Plus))
      return error(Parser.getTok().getLoc(),
                   "postincrement memory operands are not supported");
    if (!Parser.getTok().is(AsmToken::RBrac))
      return error(Parser.getTok().getLoc(),
                   "expected ']' after compact address register");
    Parser.Lex();
    return false;
  }

  static std::optional<unsigned> scalarRegisterIndex(MCRegister Reg) {
    switch (Reg.id()) {
    case AVM::R0: return 0;
    case AVM::R1: return 1;
    case AVM::R2: return 2;
    case AVM::R3: return 3;
    case AVM::R4: return 4;
    case AVM::R5: return 5;
    case AVM::R6: return 6;
    case AVM::R7: return 7;
    default: return std::nullopt;
    }
  }

  static std::optional<unsigned> programPairIndex(MCRegister Reg) {
    switch (Reg.id()) {
    case AVM::R0R1: return 0;
    case AVM::R2R3: return 1;
    case AVM::R4R5: return 2;
    case AVM::R6R7: return 3;
    default: return std::nullopt;
    }
  }

  bool parseProgramMemory(MCRegister &Address, bool &PostIncrement) {
    if (!Parser.getTok().is(AsmToken::LBrac))
      return error(Parser.getTok().getLoc(),
                   "expected program memory operand '[qN]' or '[qN+]'");
    Parser.Lex();
    if (parseProgramPair(Address))
      return true;
    PostIncrement = Parser.getTok().is(AsmToken::Plus);
    if (PostIncrement)
      Parser.Lex();
    if (!Parser.getTok().is(AsmToken::RBrac))
      return error(Parser.getTok().getLoc(),
                   "expected ']' after program address register");
    Parser.Lex();
    return false;
  }

  bool parseProgramLoad(unsigned OrdinaryOpcode, unsigned PostOpcode,
                        bool IsPair, StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands) {
    MCRegister Destination, Address;
    if ((IsPair ? parseProgramPair(Destination)
                : parseAbsoluteDataReg(Destination)) ||
        Parser.parseComma())
      return true;
    bool PostIncrement = false;
    if (parseProgramMemory(Address, PostIncrement))
      return true;
    if (PostIncrement && !PostOpcode)
      return error(NameLoc, "this program load does not support postincrement");

    if (PostIncrement) {
      if (IsPair) {
        if (Destination == Address)
          return error(NameLoc,
                       "postincrement destination must not overlap address pair");
      } else {
        const auto Data = scalarRegisterIndex(Destination);
        const auto Pair = programPairIndex(Address);
        if (!Data || !Pair || *Data / 2 == *Pair)
          return error(NameLoc,
                       "postincrement destination must not overlap address pair");
      }
    }

    MCInst Inst;
    Inst.setOpcode(PostIncrement ? PostOpcode : OrdinaryOpcode);
    Inst.addOperand(MCOperand::createReg(Destination));
    Inst.addOperand(MCOperand::createReg(Address));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseColdMemory(MCRegister &Address) {
    if (!Parser.getTok().is(AsmToken::LBrac))
      return error(Parser.getTok().getLoc(),
                   "expected data memory operand '[rN]'");
    Parser.Lex();
    if (parseStackReg(Address))
      return true;
    if (Parser.getTok().is(AsmToken::Plus))
      return error(Parser.getTok().getLoc(),
                   "postincrement memory operands are not supported");
    if (!Parser.getTok().is(AsmToken::RBrac))
      return error(Parser.getTok().getLoc(),
                   "expected ']' after data address register");
    Parser.Lex();
    return false;
  }

  bool parseCold32(unsigned Opcode, bool IsStore, bool IsCompare,
                   StringRef Name, SMLoc NameLoc,
                   OperandVector &Operands) {
    MCRegister First, Second;
    if (IsCompare) {
      if (parseProgramPair(First) || Parser.parseComma() ||
          parseProgramPair(Second))
        return true;
    } else if (IsStore) {
      if (parseColdMemory(First) || Parser.parseComma() ||
          parseProgramPair(Second))
        return true;
    } else {
      if (parseProgramPair(First) || Parser.parseComma() ||
          parseColdMemory(Second))
        return true;
    }
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(First));
    Inst.addOperand(MCOperand::createReg(Second));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseCompactPair(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands) {
    MCRegister First, Second;
    if (parseCompactReg(First))
      return true;
    if (Parser.parseComma())
      return true;
    if (parseCompactReg(Second))
      return true;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(First));
    Inst.addOperand(MCOperand::createReg(Second));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseFullReg(MCRegister &Reg) {
    const AsmToken &Tok = Parser.getTok();
    if (!Tok.is(AsmToken::Identifier))
      return error(Tok.getLoc(), "expected full register r0-r7");
    Reg = StringSwitch<MCRegister>(Tok.getIdentifier().lower())
              .Case("r0", AVM::R0).Case("r1", AVM::R1)
              .Case("r2", AVM::R2).Case("r3", AVM::R3)
              .Case("r4", AVM::R4).Case("r5", AVM::R5)
              .Case("r6", AVM::R6).Case("r7", AVM::R7)
              .Default(MCRegister());
    if (!Reg)
      return error(Tok.getLoc(), "expected full register r0-r7");
    Parser.Lex();
    return false;
  }

  static std::optional<unsigned> pair48Index(MCRegister Left,
                                              MCRegister Right) {
    const unsigned L = Left.id() - AVM::R0;
    const unsigned R = Right.id() - AVM::R0;
    if (L > 7 || R > 7 || (L >= 4 && R >= 4))
      return std::nullopt;
    return L < 4 ? 8 * L + R : 0x20 + 4 * (L - 4) + R;
  }

  bool parseFullMove(StringRef Name, SMLoc NameLoc,
                     OperandVector &Operands) {
    MCRegister Destination, Source;
    if (parseFullReg(Destination) || Parser.parseComma() ||
        parseFullReg(Source))
      return true;
    if (!pair48Index(Destination, Source))
      return error(NameLoc,
                   "full-register MOV pairing is not encodable; use compact cN spelling");
    MCInst Inst;
    Inst.setOpcode(AVM::MOV_RR);
    Inst.addOperand(MCOperand::createReg(Destination));
    Inst.addOperand(MCOperand::createReg(Source));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseFullArithmetic(unsigned Opcode, StringRef Mnemonic,
                           StringRef Name, SMLoc NameLoc,
                           OperandVector &Operands) {
    MCRegister Destination, Source;
    if (parseFullReg(Destination) || Parser.parseComma() ||
        parseFullReg(Source))
      return true;
    if (!pair48Index(Destination, Source))
      return error(NameLoc, (Mnemonic +
                   " full-register pairing is not encodable; use compact cN spelling").str());
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Destination));
    Inst.addOperand(MCOperand::createReg(Source));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseFullMultiply16(StringRef Name, SMLoc NameLoc,
                           OperandVector &Operands) {
    MCRegister Destination, Source;
    if (parseFullReg(Destination) || Parser.parseComma() ||
        parseFullReg(Source))
      return true;
    MCInst Inst;
    Inst.setOpcode(AVM::MUL16);
    Inst.addOperand(MCOperand::createReg(Destination));
    Inst.addOperand(MCOperand::createReg(Source));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseFullBitwise(unsigned FullOpcode, unsigned CompactOpcode,
                        StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands) {
    MCRegister Destination, Source;
    if (parseFullReg(Destination) || Parser.parseComma() ||
        parseFullReg(Source))
      return true;
    const bool CompactPair = Destination.id() >= AVM::R4 &&
                             Source.id() >= AVM::R4;
    MCInst Inst;
    Inst.setOpcode(CompactPair ? CompactOpcode : FullOpcode);
    Inst.addOperand(MCOperand::createReg(Destination));
    Inst.addOperand(MCOperand::createReg(Source));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseFullCompare(StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands) {
    MCRegister Left, Right;
    if (parseFullReg(Left) || Parser.parseComma() || parseFullReg(Right))
      return true;
    if (!pair48Index(Left, Right))
      return error(NameLoc,
                   "cmp full-register pairing is not encodable; use compact cN spelling");
    MCInst Inst;
    Inst.setOpcode(AVM::CMP_RR);
    Inst.addOperand(MCOperand::createReg(Left));
    Inst.addOperand(MCOperand::createReg(Right));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseF1FullReg(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                      OperandVector &Operands) {
    MCRegister Reg;
    if (parseFullReg(Reg))
      return true;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Reg));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseF7PairArithmetic(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                              OperandVector &Operands) {
    MCRegister Destination, Source;
    if (parseProgramPair(Destination) || Parser.parseComma() ||
        parseProgramPair(Source))
      return true;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Destination));
    Inst.addOperand(MCOperand::createReg(Source));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseF7PairUnary(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands) {
    MCRegister Reg;
    if (parseProgramPair(Reg))
      return true;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Reg));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseF4FullReg(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                      OperandVector &Operands) {
    MCRegister Reg;
    if (parseFullReg(Reg))
      return true;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Reg));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseCSet(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                 OperandVector &Operands) {
    MCRegister Reg;
    if (parseFullReg(Reg))
      return true;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Reg));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseConditionalMove(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                             OperandVector &Operands) {
    MCRegister Destination, Source;
    if (parseFullReg(Destination) || Parser.parseComma() ||
        parseFullReg(Source))
      return true;
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Destination));
    Inst.addOperand(MCOperand::createReg(Source));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseCompactImmediate(unsigned Opcode, bool IsSigned, unsigned Bits,
                             StringRef Name, SMLoc NameLoc,
                             OperandVector &Operands) {
    MCRegister Reg;
    if (parseCompactReg(Reg))
      return true;
    if (Parser.parseComma())
      return true;

    const MCExpr *Expr = nullptr;
    SMLoc ExprLoc;
    if (!IsSigned && (Bits == 8 || Bits == 16)) {
      if (parseLoadableImmediate(Bits, Expr, ExprLoc))
        return true;
      MCInst Inst;
      Inst.setOpcode(Opcode);
      Inst.addOperand(MCOperand::createReg(Reg));
      addExpr(Inst, Expr);
      return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                               Operands, Name, NameLoc);
    }
    ExprLoc = Parser.getTok().getLoc();
    if (Parser.parseExpression(Expr)) return true;
    int64_t Value = 0;
    if (!Expr->evaluateAsAbsolute(Value))
      return error(ExprLoc, "immediate expression must be fully resolvable");

    const int64_t Min = IsSigned ? -(int64_t(1) << (Bits - 1)) : 0;
    const int64_t Max = IsSigned ? (int64_t(1) << (Bits - 1)) - 1
                                 : (int64_t(1) << Bits) - 1;
    if (Value < Min || Value > Max)
      return error(ExprLoc, "immediate is out of range");

    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Reg));
    Inst.addOperand(MCOperand::createImm(Value));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseCompactMemoryInstruction(unsigned Opcode, bool IsStore,
                                     StringRef Name, SMLoc NameLoc,
                                     OperandVector &Operands) {
    MCRegister Data, Address;
    if (IsStore) {
      if (parseCompactMemory(Address))
        return true;
      if (Parser.parseComma())
        return true;
      if (parseCompactReg(Data))
        return true;
    } else {
      if (parseCompactReg(Data))
        return true;
      if (Parser.parseComma())
        return true;
      if (parseCompactMemory(Address))
        return true;
    }
    MCInst Inst;
    Inst.setOpcode(Opcode);
    if (IsStore) {
      Inst.addOperand(MCOperand::createReg(Address));
      Inst.addOperand(MCOperand::createReg(Data));
    } else {
      Inst.addOperand(MCOperand::createReg(Data));
      Inst.addOperand(MCOperand::createReg(Address));
    }
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseF3Multiply(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                       OperandVector &Operands) {
    return parseCompactPair(Opcode, Name, NameLoc, Operands);
  }

  bool parseSpelledDataReg(MCRegister &Reg, bool &IsFull) {
    const AsmToken &Tok = Parser.getTok();
    if (!Tok.is(AsmToken::Identifier))
      return parseArchitecturalReg(Reg);
    if (Tok.getIdentifier().starts_with_insensitive("r")) {
      IsFull = true;
      return parseStackReg(Reg);
    }
    if (Tok.getIdentifier().starts_with_insensitive("c")) {
      IsFull = false;
      return parseCompactReg(Reg);
    }
    return error(Tok.getLoc(),
                 "expected full register r0-r7 or compact register c0-c3");
  }

  bool parseSpelledDataMemory(MCRegister &Reg, bool &IsFull,
                              bool &PostIncrement,
                              std::optional<bool> ExpectedClass = std::nullopt,
                              bool AllowCompactPostIncrement = false) {
    if (!Parser.getTok().is(AsmToken::LBrac))
      return error(Parser.getTok().getLoc(),
                   ExpectedClass && *ExpectedClass
                       ? "expected data memory operand '[rN]'"
                       : "expected compact memory operand '[cN]'");
    Parser.Lex();
    if (ExpectedClass) {
      IsFull = *ExpectedClass;
      if (IsFull ? parseStackReg(Reg) : parseCompactReg(Reg))
        return true;
    } else if (parseSpelledDataReg(Reg, IsFull)) {
      return true;
    }
    PostIncrement = Parser.getTok().is(AsmToken::Plus);
    if (PostIncrement) {
      if (!IsFull && !AllowCompactPostIncrement)
        return error(Parser.getTok().getLoc(),
                     "postincrement memory operands are not supported");
      Parser.Lex();
    }
    if (!Parser.getTok().is(AsmToken::RBrac))
      return error(Parser.getTok().getLoc(),
                   "expected ']' after data address register");
    Parser.Lex();
    return false;
  }

  bool parseOverloadedMemoryInstruction(unsigned CompactOpcode,
                                        unsigned GeneralOpcode,
                                        unsigned GeneralPostOpcode,
                                        bool IsStore, StringRef Name,
                                        SMLoc NameLoc,
                                        OperandVector &Operands,
                                        unsigned MixedOpcode = 0,
                                        unsigned CompactPostOpcode = 0) {
    MCRegister Data, Address;
    bool DataIsFull = false, AddressIsFull = false, PostIncrement = false;
    if (IsStore) {
      if (parseSpelledDataMemory(Address, AddressIsFull, PostIncrement,
                                 std::nullopt, CompactPostOpcode != 0) ||
          Parser.parseComma() || parseSpelledDataReg(Data, DataIsFull))
        return true;
    } else {
      if (parseSpelledDataReg(Data, DataIsFull) || Parser.parseComma() ||
          parseSpelledDataMemory(Address, AddressIsFull, PostIncrement,
                                 DataIsFull))
        return true;
    }
    if (!AddressIsFull && DataIsFull && MixedOpcode && !PostIncrement) {
      const auto Source = scalarRegisterIndex(Data);
      if (!Source || *Source > 3)
        return error(NameLoc, "expected source register r0-r3");
    } else if (DataIsFull != AddressIsFull &&
               !(CompactPostOpcode && PostIncrement && !AddressIsFull))
      return error(NameLoc, "expected compact register c0-c3");
    if (!AddressIsFull && PostIncrement && CompactPostOpcode && !DataIsFull)
      return error(NameLoc, "expected full register r0-r7");
    if (!AddressIsFull && PostIncrement && !CompactPostOpcode)
      return error(NameLoc, "compact memory operands do not support postincrement");
    if (DataIsFull && !IsStore && PostIncrement && Data == Address)
      return error(NameLoc,
                   "postincrement destination must not overlap address register");

    MCInst Inst;
    Inst.setOpcode(!AddressIsFull && PostIncrement && CompactPostOpcode
                       ? CompactPostOpcode
                       : !AddressIsFull && DataIsFull && MixedOpcode
                       ? MixedOpcode
                       : DataIsFull
                       ? (PostIncrement ? GeneralPostOpcode : GeneralOpcode)
                       : CompactOpcode);
    if (IsStore) {
      Inst.addOperand(MCOperand::createReg(Address));
      Inst.addOperand(MCOperand::createReg(Data));
    } else {
      Inst.addOperand(MCOperand::createReg(Data));
      Inst.addOperand(MCOperand::createReg(Address));
    }
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseF5LoadInstruction(unsigned F5Opcode, unsigned CompactPostOpcode,
                              unsigned GeneralOpcode,
                              unsigned GeneralPostOpcode, StringRef Name,
                              SMLoc NameLoc, OperandVector &Operands) {
    MCRegister Data, Address;
    bool DataIsFull = false;
    bool AddressIsFull = false, PostIncrement = false;
    if (parseSpelledDataReg(Data, DataIsFull) || !DataIsFull ||
        Parser.parseComma() ||
        parseSpelledDataMemory(Address, AddressIsFull, PostIncrement,
                               std::nullopt, CompactPostOpcode != 0))
      return true;
    const auto Index = scalarRegisterIndex(Data);
    if (!AddressIsFull && !PostIncrement && (!Index || *Index > 3))
      return error(NameLoc, "expected destination register r0-r3");
    if (!AddressIsFull && PostIncrement && Data == Address)
      return error(NameLoc,
                   "postincrement destination must not overlap address register");
    MCInst Inst;
    Inst.setOpcode(!AddressIsFull
                       ? (PostIncrement ? CompactPostOpcode : F5Opcode)
                       : (PostIncrement ? GeneralPostOpcode : GeneralOpcode));
    Inst.addOperand(MCOperand::createReg(Data));
    Inst.addOperand(MCOperand::createReg(Address));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseClr(StringRef Name, SMLoc NameLoc, OperandVector &Operands) {
    MCRegister Reg;
    if (parseCompactReg(Reg))
      return true;
    MCInst Inst;
    Inst.setOpcode(AVM::XOR);
    Inst.addOperand(MCOperand::createReg(Reg));
    Inst.addOperand(MCOperand::createReg(Reg));
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseNop(StringRef Name, SMLoc NameLoc, OperandVector &Operands) {
    MCInst Inst;
    Inst.setOpcode(AVM::MOV);
    Inst.addOperand(MCOperand::createReg(AVM::R4));
    Inst.addOperand(MCOperand::createReg(AVM::R4));
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

  bool parsePrimaryExpr(const MCExpr *&Res, SMLoc &EndLoc) override {
    if (!Parser.getTok().is(AsmToken::Percent))
      return MCTargetAsmParser::parsePrimaryExpr(Res, EndLoc);
    SMLoc Loc = Parser.getTok().getLoc();
    Parser.Lex();
    if (!Parser.getTok().is(AsmToken::Identifier))
      return error(Loc, "expected AVM expression modifier name after '%'");
    StringRef Name = Parser.getTok().getIdentifier();
    uint16_t Kind = StringSwitch<uint16_t>(Name.lower())
        .Case("lo16", AVM::VK_AVM_LO16)
        .Case("hi8", AVM::VK_AVM_HI8)
        .Case("prog24", AVM::VK_AVM_PROG24)
        .Default(0);
    if (!Kind)
      return error(Loc, "unknown AVM expression modifier");
    Parser.Lex();
    if (!Parser.getTok().is(AsmToken::LParen))
      return error(Parser.getTok().getLoc(), "expected '(' after AVM expression modifier");
    Parser.Lex();
    if (Parser.getTok().is(AsmToken::RParen))
      return error(Parser.getTok().getLoc(), "expected expression inside AVM modifier");
    const MCExpr *Inner = nullptr;
    if (Parser.parseExpression(Inner)) return true;
    if (isa<MCSpecifierExpr>(Inner))
      return error(Loc, "nested AVM expression modifiers are not supported");
    if (!Parser.getTok().is(AsmToken::RParen))
      return error(Parser.getTok().getLoc(), "expected ')' after AVM expression modifier");
    EndLoc = Parser.getTok().getEndLoc();
    Parser.Lex();
    Res = MCSpecifierExpr::create(Inner, Kind, getContext(), Loc);
    return false;
  }

  ParseStatus parseDirective(AsmToken Directive) override {
    if (Directive.getString().equals_insensitive(".word")) {
      auto ParseOne = [&]() -> bool {
        const MCExpr *Expr = nullptr;
        SMLoc Loc = Parser.getTok().getLoc();
        if (Parser.parseExpression(Expr)) return true;
        Parser.getStreamer().emitValue(Expr, 2, Loc);
        return false;
      };
      if (Parser.parseMany(ParseOne)) return ParseStatus::Failure;
      return ParseStatus::Success;
    }
    if (!Directive.getString().equals_insensitive(".progptr"))
      return ParseStatus::NoMatch;
    const MCExpr *Expr = nullptr;
    SMLoc Loc;
    if (parseProgramAddressExpr(Expr, Loc) ||
        Parser.parseEOL("unexpected token after .progptr"))
      return ParseStatus::Failure;
    const auto *Spec = dyn_cast<MCSpecifierExpr>(Expr);
    if (Spec && Spec->getSpecifier() != AVM::VK_AVM_PROG24) {
      error(Loc, ".progptr only accepts %prog24() modifiers");
      return ParseStatus::Failure;
    }
    MCInst Inst;
    Inst.setOpcode(AVM::PROGPTR);
    addExpr(Inst, Expr);
    if (auto *Section = static_cast<MCSectionELF *>(Parser.getStreamer().getCurrentSectionOnly())) {
      if (Section->getType() == ELF::SHT_INIT_ARRAY ||
          Section->getType() == ELF::SHT_FINI_ARRAY)
        Section->setEntrySize(3);
    }
    Parser.getStreamer().emitInstruction(Inst, getSTI());
    return ParseStatus::Success;
  }

  bool parseInstruction(ParseInstructionInfo &, StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands) override {
    Pending.reset();
    std::string Lower = Name.lower();
    if (Lower == "mov") {
      if (Parser.getTok().is(AsmToken::Identifier) &&
          Parser.getTok().getIdentifier().starts_with_insensitive("r"))
        return parseFullMove(Name, NameLoc, Operands);
      return parseCompactPair(AVM::MOV, Name, NameLoc, Operands);
    }
    if (Lower == "zext8")
      return parseF1FullReg(AVM::ZEXT8, Name, NameLoc, Operands);
    if (Lower == "swap8")
      return parseF1FullReg(AVM::SWAP8, Name, NameLoc, Operands);
    if (Lower == "getsp")
      return parseF1FullReg(AVM::GETSP, Name, NameLoc, Operands);
    if (Lower == "setsp")
      return parseF1FullReg(AVM::SETSP, Name, NameLoc, Operands);
    if (Lower == "lsl16.1")
      return parseF4FullReg(AVM::LSL16_1, Name, NameLoc, Operands);
    if (Lower == "lsr16.1")
      return parseF4FullReg(AVM::LSR16_1, Name, NameLoc, Operands);
    if (Lower == "asr16.1")
      return parseF4FullReg(AVM::ASR16_1, Name, NameLoc, Operands);
    if (Lower == "shl16v")
      return parseCompactPair(AVM::SHL16V, Name, NameLoc, Operands);
    if (Lower == "lsr16v")
      return parseCompactPair(AVM::LSR16V, Name, NameLoc, Operands);
    if (Lower == "asr16v")
      return parseCompactPair(AVM::ASR16V, Name, NameLoc, Operands);
    if (Lower == "not16")
      return parseF4FullReg(AVM::NOT16, Name, NameLoc, Operands);
    if (Lower == "tst8")
      return parseF4FullReg(AVM::TST8, Name, NameLoc, Operands);
    if (Lower == "inc16")
      return parseF4FullReg(AVM::INC16, Name, NameLoc, Operands);
    if (Lower == "dec16")
      return parseF4FullReg(AVM::DEC16, Name, NameLoc, Operands);
    if (Lower == "bswap16")
      return parseF4FullReg(AVM::BSWAP16, Name, NameLoc, Operands);
    if (Lower == "tst16")
      return parseF4FullReg(AVM::TST16, Name, NameLoc, Operands);
    if (Lower == "mul8")
      return parseCompactPair(AVM::MUL8, Name, NameLoc, Operands);
    if (Lower == "mul16")
      return parseFullMultiply16(Name, NameLoc, Operands);
    if (Lower == "sext8")
      return parseF4FullReg(AVM::SEXT8, Name, NameLoc, Operands);
    if (Lower == "neg16")
      return parseF4FullReg(AVM::NEG16, Name, NameLoc, Operands);
    if (Lower == "cset.eq")
      return parseCSet(AVM::CSET_EQ, Name, NameLoc, Operands);
    if (Lower == "cset.ne")
      return parseCSet(AVM::CSET_NE, Name, NameLoc, Operands);
    if (Lower == "cset.ult")
      return parseCSet(AVM::CSET_ULT, Name, NameLoc, Operands);
    if (Lower == "cset.uge")
      return parseCSet(AVM::CSET_UGE, Name, NameLoc, Operands);
    if (Lower == "cset.slt")
      return parseCSet(AVM::CSET_SLT, Name, NameLoc, Operands);
    if (Lower == "cset.sge")
      return parseCSet(AVM::CSET_SGE, Name, NameLoc, Operands);
    if (Lower == "cmov.eq")
      return parseConditionalMove(AVM::CMOV_EQ, Name, NameLoc, Operands);
    if (Lower == "cmov.ne")
      return parseConditionalMove(AVM::CMOV_NE, Name, NameLoc, Operands);
    if (Lower == "cmov.ult")
      return parseConditionalMove(AVM::CMOV_ULT, Name, NameLoc, Operands);
    if (Lower == "cmov.uge")
      return parseConditionalMove(AVM::CMOV_UGE, Name, NameLoc, Operands);
    if (Lower == "cmov.slt")
      return parseConditionalMove(AVM::CMOV_SLT, Name, NameLoc, Operands);
    if (Lower == "cmov.sge")
      return parseConditionalMove(AVM::CMOV_SGE, Name, NameLoc, Operands);
    if (Lower == "add32")
      return parseF7PairArithmetic(AVM::ADD32, Name, NameLoc, Operands);
    if (Lower == "sub32")
      return parseF7PairArithmetic(AVM::SUB32, Name, NameLoc, Operands);
    if (Lower == "lsr32.1")
      return parseF7PairUnary(AVM::LSR32_1, Name, NameLoc, Operands);
    if (Lower == "asr32.1")
      return parseF7PairUnary(AVM::ASR32_1, Name, NameLoc, Operands);
    if (Lower == "bool")
      return parseF1FullReg(AVM::BOOL, Name, NameLoc, Operands);
    if (Lower == "add") {
      if (Parser.getTok().is(AsmToken::Identifier) &&
          Parser.getTok().getIdentifier().starts_with_insensitive("r"))
        return parseFullArithmetic(AVM::ADD_RR, "add", Name, NameLoc,
                                   Operands);
      return parseCompactPair(AVM::ADD, Name, NameLoc, Operands);
    }
    if (Lower == "sub") {
      if (Parser.getTok().is(AsmToken::Identifier) &&
          Parser.getTok().getIdentifier().starts_with_insensitive("r"))
        return parseFullArithmetic(AVM::SUB_RR, "sub", Name, NameLoc,
                                   Operands);
      return parseCompactPair(AVM::SUB, Name, NameLoc, Operands);
    }
    if (Lower == "cmp") {
      if (Parser.getTok().is(AsmToken::Identifier) &&
          Parser.getTok().getIdentifier().starts_with_insensitive("r"))
        return parseFullCompare(Name, NameLoc, Operands);
      return parseCompactPair(AVM::CMP, Name, NameLoc, Operands);
    }
    if (Lower == "ld8u") {
      if (Parser.getTok().is(AsmToken::Identifier) &&
          Parser.getTok().getIdentifier().starts_with_insensitive("r"))
        return parseF5LoadInstruction(AVM::F5LD8U, AVM::F7LD8U_POST,
                                      AVM::GPLD8U,
                                      AVM::GPLD8U_POST, Name, NameLoc,
                                      Operands);
      return parseOverloadedMemoryInstruction(AVM::LD8U, AVM::GPLD8U,
                                              AVM::GPLD8U_POST, false, Name,
                                              NameLoc, Operands);
    }
    if (Lower == "st8" && Parser.getTok().is(AsmToken::LBrac)) {
      // Select the compact postincrement form by its cN spelling. The
      // general overloaded parser remains responsible for [rN+] and all
      // existing ordinary forms.
      const auto Saved = Parser.getTok();
      (void)Saved;
    }
    if (Lower == "st8")
      return parseOverloadedMemoryInstruction(AVM::ST8, AVM::GPST8,
                                              AVM::GPST8_POST, true, Name,
                                              NameLoc, Operands, AVM::F3ST8,
                                              AVM::F6ST8_POST);
    if (Lower == "mulu8.w")
      return parseF3Multiply(AVM::MULU8W, Name, NameLoc, Operands);
    if (Lower == "muls8.w")
      return parseF3Multiply(AVM::MULS8W, Name, NameLoc, Operands);
    if (Lower == "mulsu8.w")
      return parseF3Multiply(AVM::MULSU8W, Name, NameLoc, Operands);
    if (Lower == "ld16") {
      if (Parser.getTok().is(AsmToken::Identifier) &&
          Parser.getTok().getIdentifier().starts_with_insensitive("r"))
        return parseF5LoadInstruction(AVM::F5LD16, AVM::F7LD16_POST,
                                      AVM::GPLD16,
                                      AVM::GPLD16_POST, Name, NameLoc,
                                      Operands);
      return parseOverloadedMemoryInstruction(AVM::LD16, AVM::GPLD16,
                                              AVM::GPLD16_POST, false, Name,
                                              NameLoc, Operands);
    }
    if (Lower == "st16")
      return parseOverloadedMemoryInstruction(AVM::ST16, AVM::GPST16,
                                              AVM::GPST16_POST, true, Name,
                                              NameLoc, Operands, AVM::F5ST16,
                                              AVM::F7ST16_POST);
    if (Lower == "ldm8u")
      return parseAbsoluteDataInstruction(AVM::LDM8U, false, Name, NameLoc,
                                          Operands);
    if (Lower == "stm8")
      return parseAbsoluteDataInstruction(AVM::STM8, true, Name, NameLoc,
                                          Operands);
    if (Lower == "ldm16")
      return parseAbsoluteDataInstruction(AVM::LDM16, false, Name, NameLoc,
                                          Operands);
    if (Lower == "stm16")
      return parseAbsoluteDataInstruction(AVM::STM16, true, Name, NameLoc,
                                          Operands);
    if (Lower == "ldp8u")
      return parseProgramLoad(AVM::LDP8U, AVM::LDP8U_POST, false, Name,
                              NameLoc, Operands);
    if (Lower == "ldp8s")
      return parseProgramLoad(AVM::LDP8S, 0, false, Name, NameLoc, Operands);
    if (Lower == "ldp16")
      return parseProgramLoad(AVM::LDP16, AVM::LDP16_POST, false, Name,
                              NameLoc, Operands);
    if (Lower == "ldp24")
      return parseProgramLoad(AVM::LDP24, AVM::LDP24_POST, true, Name,
                              NameLoc, Operands);
    if (Lower == "ldp32")
      return parseProgramLoad(AVM::LDP32, AVM::LDP32_POST, true, Name,
                              NameLoc, Operands);
    if (Lower == "cmp32")
      return parseCold32(AVM::CMP32, false, true, Name, NameLoc, Operands);
    if (Lower == "ld32")
      return parseCold32(AVM::LD32, false, false, Name, NameLoc, Operands);
    if (Lower == "st32")
      return parseCold32(AVM::ST32, true, false, Name, NameLoc, Operands);
    if (Lower == "and") {
      if (Parser.getTok().is(AsmToken::Identifier) &&
          Parser.getTok().getIdentifier().starts_with_insensitive("r"))
        return parseFullBitwise(AVM::AND_RR, AVM::AND, Name, NameLoc,
                                Operands);
      return parseCompactPair(AVM::AND, Name, NameLoc, Operands);
    }
    if (Lower == "or") {
      if (Parser.getTok().is(AsmToken::Identifier) &&
          Parser.getTok().getIdentifier().starts_with_insensitive("r"))
        return parseFullBitwise(AVM::OR_RR, AVM::OR, Name, NameLoc,
                                Operands);
      return parseCompactPair(AVM::OR, Name, NameLoc, Operands);
    }
    if (Lower == "xor") {
      if (Parser.getTok().is(AsmToken::Identifier) &&
          Parser.getTok().getIdentifier().starts_with_insensitive("r"))
        return parseFullBitwise(AVM::XOR_RR, AVM::XOR, Name, NameLoc,
                                Operands);
      return parseCompactPair(AVM::XOR, Name, NameLoc, Operands);
    }
    if (Lower == "push16")
      return parseStackInstruction(AVM::PUSH16, Name, NameLoc, Operands);
    if (Lower == "pop16")
      return parseStackInstruction(AVM::POP16, Name, NameLoc, Operands);
    if (Lower == "ldi8" && Parser.getTok().is(AsmToken::Identifier) &&
        (Parser.getTok().getIdentifier().equals_insensitive("r0") ||
         Parser.getTok().getIdentifier().equals_insensitive("r1") ||
         Parser.getTok().getIdentifier().equals_insensitive("r2") ||
         Parser.getTok().getIdentifier().equals_insensitive("r3")))
      return parseColdImmediate(AVM::COLDLDI8, false, 8, Name, NameLoc, Operands);
    if (Lower == "ldi8")
      return parseCompactImmediate(AVM::LDI8, false, 8, Name, NameLoc,
                                   Operands);
    if (Lower == "ldi16" && Parser.getTok().is(AsmToken::Identifier) &&
        (Parser.getTok().getIdentifier().equals_insensitive("r0") ||
         Parser.getTok().getIdentifier().equals_insensitive("r1") ||
         Parser.getTok().getIdentifier().equals_insensitive("r2") ||
         Parser.getTok().getIdentifier().equals_insensitive("r3")))
      return parseColdImmediate(AVM::COLDLDI16, false, 16, Name, NameLoc, Operands);
    if (Lower == "ldi16")
      return parseCompactImmediate(AVM::LDI16, false, 16, Name, NameLoc,
                                   Operands);
    if (Lower == "addi.s8" && Parser.getTok().is(AsmToken::Identifier) &&
        (Parser.getTok().getIdentifier().equals_insensitive("r0") ||
         Parser.getTok().getIdentifier().equals_insensitive("r1") ||
         Parser.getTok().getIdentifier().equals_insensitive("r2") ||
         Parser.getTok().getIdentifier().equals_insensitive("r3")))
      return parseColdImmediate(AVM::COLDADDIS8, true, 8, Name, NameLoc, Operands);
    if (Lower == "addi.s8")
      return parseCompactImmediate(AVM::ADDIS8, true, 8, Name, NameLoc,
                                   Operands);
    if (Lower == "cmpi.s8" && Parser.getTok().is(AsmToken::Identifier) &&
        (Parser.getTok().getIdentifier().equals_insensitive("r0") ||
         Parser.getTok().getIdentifier().equals_insensitive("r1") ||
         Parser.getTok().getIdentifier().equals_insensitive("r2") ||
         Parser.getTok().getIdentifier().equals_insensitive("r3")))
      return parseColdImmediate(AVM::COLDCMPIS8, true, 8, Name, NameLoc, Operands);
    if (Lower == "cmpi.s8")
      return parseCompactImmediate(AVM::CMPIS8, true, 8, Name, NameLoc,
                                   Operands);
    if (Lower == "leasp") return parseLEASP(Name, NameLoc, Operands);
    if (Lower == "ldsp8u") return parseLDSP8U(Name, NameLoc, Operands);
    if (Lower == "ldsp8s") return parseSPMemoryInstruction(AVM::LDSP8S, false, Name, NameLoc, Operands);
    if (Lower == "stsp8") return parseSTSP8(Name, NameLoc, Operands);
    if (Lower == "ldsp16") return parseSPWordInstruction(Name, NameLoc, Operands, false);
    if (Lower == "stsp16") return parseSPWordInstruction(Name, NameLoc, Operands, true);
    if (Lower == "nop")
      return parseNop(Name, NameLoc, Operands);
    if (Lower == "clr")
      return parseClr(Name, NameLoc, Operands);
    if (Lower == "jmpf")
      return parseFarTransfer(AVM::JMPF, Name, NameLoc, Operands);
    if (Lower == "callf")
      return parseFarTransfer(AVM::CALLF, Name, NameLoc, Operands);
    if (Lower == "jmp")
      return parseRelaxableTransfer(AVM::RELAX_JMP, Name, NameLoc, Operands);
    if (Lower == "call")
      return parseRelaxableTransfer(AVM::RELAX_CALL, Name, NameLoc, Operands);
    if (Lower == "br.eq")
      return parseRelaxableTransfer(AVM::RELAX_BR_EQ, Name, NameLoc, Operands);
    if (Lower == "br.ne")
      return parseRelaxableTransfer(AVM::RELAX_BR_NE, Name, NameLoc, Operands);
    if (Lower == "br.ult")
      return parseRelaxableTransfer(AVM::RELAX_BR_ULT, Name, NameLoc, Operands);
    if (Lower == "br.uge")
      return parseRelaxableTransfer(AVM::RELAX_BR_UGE, Name, NameLoc, Operands);
    if (Lower == "br.slt")
      return parseRelaxableTransfer(AVM::RELAX_BR_SLT, Name, NameLoc, Operands);
    if (Lower == "br.sge")
      return parseRelaxableTransfer(AVM::RELAX_BR_SGE, Name, NameLoc, Operands);
    if (Lower == "jmp16")
      return parseRel16Control(AVM::JMP16, Name, NameLoc, Operands);
    if (Lower == "call16")
      return parseRel16Control(AVM::CALL16, Name, NameLoc, Operands);
    if (Lower == "jmpp")
      return parseProgramPairTransfer(AVM::JMPP, Name, NameLoc, Operands);
    if (Lower == "callp")
      return parseProgramPairTransfer(AVM::CALLP, Name, NameLoc, Operands);
    if (Lower == "ret")
      return parseOperandless(AVM::RET, Name, NameLoc, Operands);
    if (Lower == "breq") return parseRel8Control(AVM::BREQ, true, Name, NameLoc, Operands);
    if (Lower == "brne") return parseRel8Control(AVM::BRNE, true, Name, NameLoc, Operands);
    if (Lower == "brult") return parseRel8Control(AVM::BRULT, true, Name, NameLoc, Operands);
    if (Lower == "brslt") return parseRel8Control(AVM::BRSLT, true, Name, NameLoc, Operands);
    if (Lower == "bruge") return parseRel8Control(AVM::BRUGE, true, Name, NameLoc, Operands);
    if (Lower == "brsge") return parseRel8Control(AVM::BRSGE, true, Name, NameLoc, Operands);
    if (Lower == "jmp8") return parseRel8Control(AVM::JMP8, true, Name, NameLoc, Operands);
    if (Lower == "call8") return parseRel8Control(AVM::CALL8, true, Name, NameLoc, Operands);
    if (Lower == "adjsp") return parseSignedImmediate(AVM::ADJSP, Name, NameLoc, Operands);
    if (Lower == "sys") return parseService(Name, NameLoc, Operands);
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
