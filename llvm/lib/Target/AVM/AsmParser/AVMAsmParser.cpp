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

  enum class MemoryKind { Data, Stack, Program };
  struct MemoryOperand {
    MemoryKind Kind = MemoryKind::Data;
    MCRegister Base;
    const MCExpr *Disp = nullptr;
    bool PostIncrement = false;
  };

  bool error(SMLoc Loc, const Twine &Message) {
    return Parser.Error(Loc, Message);
  }

  static bool isCompact(MCRegister Reg) {
    return Reg == AVM::R4 || Reg == AVM::R5 || Reg == AVM::R6 ||
           Reg == AVM::R7 || Reg == AVM::B4 || Reg == AVM::B5 ||
           Reg == AVM::B6 || Reg == AVM::B7;
  }

  static void addExpr(MCInst &Inst, const MCExpr *Expr) {
    if (const auto *CE = dyn_cast<MCConstantExpr>(Expr))
      Inst.addOperand(MCOperand::createImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
  }

  bool parseGPR(MCRegister &Reg, SMLoc *Start = nullptr,
                SMLoc *End = nullptr) {
    const AsmToken &Tok = Parser.getTok();
    if (!Tok.is(AsmToken::Identifier))
      return Parser.Error(Tok.getLoc(), "expected AVM register");
    StringRef Name = Tok.getIdentifier();
    std::string Lower = Name.lower();
    Reg = StringSwitch<MCRegister>(Lower)
              .Case("r0", AVM::R0).Case("r1", AVM::R1)
              .Case("r2", AVM::R2).Case("r3", AVM::R3)
              .Case("r4", AVM::R4).Case("r5", AVM::R5)
              .Case("r6", AVM::R6).Case("r7", AVM::R7)
              .Case("c0", AVM::R4).Case("c1", AVM::R5)
              .Case("c2", AVM::R6).Case("c3", AVM::R7)
              .Case("a", AVM::R4)
              .Case("b0", AVM::B0).Case("b1", AVM::B1)
              .Case("b2", AVM::B2).Case("b3", AVM::B3)
              .Case("b4", AVM::B4).Case("b5", AVM::B5)
              .Case("b6", AVM::B6).Case("b7", AVM::B7)
              .Default(MCRegister());
    if (!Reg)
      return Parser.Error(Tok.getLoc(), "unknown AVM register");
    if (Start)
      *Start = Tok.getLoc();
    if (End)
      *End = Tok.getEndLoc();
    Parser.Lex();
    return false;
  }

  bool parseExpressionOperand(MCInst &Inst) {
    const MCExpr *Expr = nullptr;
    if (Parser.parseExpression(Expr))
      return true;
    addExpr(Inst, Expr);
    return false;
  }

  bool parseMemory(MemoryOperand &Mem) {
    SMLoc Loc = Parser.getTok().getLoc();
    if (Parser.parseToken(AsmToken::LBrac, "expected '['"))
      return true;

    if (Parser.getTok().is(AsmToken::Identifier) &&
        Parser.getTok().getIdentifier().equals_insensitive("pb")) {
      Mem.Kind = MemoryKind::Program;
      Parser.Lex();
      if (Parser.parseToken(AsmToken::Colon, "expected ':' after PB"))
        return true;
      if (parseGPR(Mem.Base))
        return true;
    } else if (Parser.getTok().is(AsmToken::Identifier) &&
               Parser.getTok().getIdentifier().equals_insensitive("sp")) {
      Mem.Kind = MemoryKind::Stack;
      Mem.Base = AVM::SP;
      Parser.Lex();
    } else {
      Mem.Kind = MemoryKind::Data;
      if (parseGPR(Mem.Base))
        return true;
    }

    if (Parser.getTok().is(AsmToken::Plus)) {
      Parser.Lex();
      if (Parser.parseExpression(Mem.Disp))
        return true;
    } else if (Parser.getTok().is(AsmToken::Minus)) {
      if (Parser.parseExpression(Mem.Disp))
        return true;
    }

    if (Parser.parseToken(AsmToken::RBrac, "expected ']'"))
      return true;
    if (Parser.getTok().is(AsmToken::Plus)) {
      Mem.PostIncrement = true;
      Parser.Lex();
    }
    if (Mem.PostIncrement && Mem.Kind != MemoryKind::Data)
      return error(Loc, "postincrement is supported only in data space");
    return false;
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

  bool parseNoOperand(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                      OperandVector &Operands) {
    MCInst Inst;
    Inst.setOpcode(Opcode);
    return finishInstruction(std::move(Inst), NameLoc, Operands, Name, NameLoc);
  }

  bool parseOneReg(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                   OperandVector &Operands, bool RequireCompact = false) {
    MCRegister Reg;
    SMLoc End;
    if (parseGPR(Reg, nullptr, &End))
      return true;
    if (RequireCompact && !isCompact(Reg))
      return error(NameLoc, "instruction requires compact register c0-c3");
    MCInst Inst;
    Inst.setOpcode(Opcode);
    Inst.addOperand(MCOperand::createReg(Reg));
    return finishInstruction(std::move(Inst), End, Operands, Name, NameLoc);
  }

  bool parseRegReg(unsigned CompactOpcode, unsigned FullOpcode, StringRef Name,
                   SMLoc NameLoc, OperandVector &Operands,
                   bool DiagonalUsesFull = false) {
    MCRegister Dst, Src;
    SMLoc End;
    if (parseGPR(Dst) || Parser.parseComma() || parseGPR(Src, nullptr, &End))
      return true;
    bool Compact = isCompact(Dst) && isCompact(Src) &&
                   (!DiagonalUsesFull || Dst != Src);
    MCInst Inst;
    Inst.setOpcode(Compact ? CompactOpcode : FullOpcode);
    Inst.addOperand(MCOperand::createReg(Dst));
    Inst.addOperand(MCOperand::createReg(Src));
    return finishInstruction(std::move(Inst), End, Operands, Name, NameLoc);
  }

  bool parseRegImm(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                   OperandVector &Operands, bool CompactPreferred = false,
                   unsigned CompactOpcode = 0) {
    MCRegister Reg;
    if (parseGPR(Reg) || Parser.parseComma())
      return true;
    MCInst Inst;
    Inst.setOpcode(CompactPreferred && isCompact(Reg) ? CompactOpcode : Opcode);
    Inst.addOperand(MCOperand::createReg(Reg));
    if (parseExpressionOperand(Inst))
      return true;
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(), Operands,
                             Name, NameLoc);
  }

  bool parseBranch(unsigned Opcode, StringRef Name, SMLoc NameLoc,
                   OperandVector &Operands) {
    MCInst Inst;
    Inst.setOpcode(Opcode);
    if (parseExpressionOperand(Inst))
      return true;
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(), Operands,
                             Name, NameLoc);
  }

  bool parseCSet(StringRef Name, SMLoc NameLoc, OperandVector &Operands) {
    MCRegister Dst;
    if (parseGPR(Dst) || Parser.parseComma())
      return true;
    const AsmToken &Tok = Parser.getTok();
    if (!Tok.is(AsmToken::Identifier))
      return error(Tok.getLoc(), "expected condition code");
    std::string Lower = Tok.getIdentifier().lower();
    int CC = StringSwitch<int>(Lower)
      .Case("eq", 0).Case("ne", 1).Case("ult", 2).Case("uge", 3)
      .Case("slt", 4).Case("sge", 5).Case("ule", 6).Case("ugt", 7)
      .Default(-1);
    if (CC < 0)
      return error(Tok.getLoc(), "unknown AVM condition code");
    SMLoc End = Tok.getEndLoc();
    Parser.Lex();
    MCInst Inst;
    Inst.setOpcode(AVM::CSET);
    Inst.addOperand(MCOperand::createReg(Dst));
    Inst.addOperand(MCOperand::createImm(CC));
    return finishInstruction(std::move(Inst), End, Operands, Name, NameLoc);
  }

  bool parseLoadStore(StringRef Name, SMLoc NameLoc, OperandVector &Operands,
                      bool IsLoad, bool IsWord, bool ExplicitPost) {
    MCInst Inst;
    MemoryOperand Mem;
    MCRegister Reg;
    SMLoc End;

    if (IsLoad) {
      if (parseGPR(Reg) || Parser.parseComma() || parseMemory(Mem))
        return true;
    } else {
      if (parseMemory(Mem) || Parser.parseComma() ||
          parseGPR(Reg, nullptr, &End))
        return true;
    }

    if (Mem.Kind != MemoryKind::Data)
      return error(NameLoc, "use LDSP/STSP or LDP for non-data-space memory");
    if (ExplicitPost && !Mem.PostIncrement)
      return error(NameLoc, "postincrement instruction requires '[rN]+'");
    if (!ExplicitPost && Mem.PostIncrement)
      ExplicitPost = true;

    if (ExplicitPost) {
      Inst.setOpcode(IsLoad ? (IsWord ? AVM::LD16_POST : AVM::LD8_POST)
                            : (IsWord ? AVM::ST16_POST : AVM::ST8_POST));
      if (Mem.Disp)
        return error(NameLoc, "postincrement form cannot have a displacement");
    } else if (Mem.Disp) {
      Inst.setOpcode(IsLoad ? (IsWord ? AVM::LD16_DISP : AVM::LD8_DISP)
                            : (IsWord ? AVM::ST16_DISP : AVM::ST8_DISP));
    } else if (isCompact(Reg) && isCompact(Mem.Base)) {
      Inst.setOpcode(IsLoad ? (IsWord ? AVM::LD16C : AVM::LD8C)
                            : (IsWord ? AVM::ST16C : AVM::ST8C));
    } else {
      Inst.setOpcode(IsLoad ? (IsWord ? AVM::LD16 : AVM::LD8)
                            : (IsWord ? AVM::ST16 : AVM::ST8));
    }

    if (IsLoad) {
      Inst.addOperand(MCOperand::createReg(Reg));
      Inst.addOperand(MCOperand::createReg(Mem.Base));
      if (Mem.Disp)
        addExpr(Inst, Mem.Disp);
    } else {
      Inst.addOperand(MCOperand::createReg(Mem.Base));
      if (Mem.Disp)
        addExpr(Inst, Mem.Disp);
      Inst.addOperand(MCOperand::createReg(Reg));
    }
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseStack(StringRef Name, SMLoc NameLoc, OperandVector &Operands,
                  bool IsLoad, bool IsWord) {
    MCInst Inst;
    MemoryOperand Mem;
    MCRegister Reg;
    SMLoc End;
    if (IsLoad) {
      if (parseGPR(Reg) || Parser.parseComma() || parseMemory(Mem))
        return true;
    } else {
      if (parseMemory(Mem) || Parser.parseComma() ||
          parseGPR(Reg, nullptr, &End))
        return true;
    }
    if (Mem.Kind != MemoryKind::Stack || !Mem.Disp || Mem.PostIncrement)
      return error(NameLoc, "expected stack-relative operand '[sp+offset]'");

    if (IsLoad)
      Inst.setOpcode(IsWord ? AVM::LDSP16 : AVM::LDSP8);
    else
      Inst.setOpcode(IsWord ? AVM::STSP16 : AVM::STSP8);

    if (IsLoad) {
      Inst.addOperand(MCOperand::createReg(Reg));
      addExpr(Inst, Mem.Disp);
    } else {
      addExpr(Inst, Mem.Disp);
      Inst.addOperand(MCOperand::createReg(Reg));
    }
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(),
                             Operands, Name, NameLoc);
  }

  bool parseDirectMemory(StringRef Name, SMLoc NameLoc,
                         OperandVector &Operands, bool IsLoad, bool IsWord) {
    MCInst Inst;
    MCRegister Reg;
    if (IsLoad) {
      if (parseGPR(Reg) || Parser.parseComma())
        return true;
      Inst.setOpcode(IsWord ? AVM::LDM16 : AVM::LDM8);
      Inst.addOperand(MCOperand::createReg(Reg));
      if (parseExpressionOperand(Inst))
        return true;
    } else {
      Inst.setOpcode(IsWord ? AVM::STM16 : AVM::STM8);
      if (parseExpressionOperand(Inst) || Parser.parseComma() || parseGPR(Reg))
        return true;
      Inst.addOperand(MCOperand::createReg(Reg));
    }
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(), Operands,
                             Name, NameLoc);
  }

  bool parseProgramLoad(StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands, bool IsWord) {
    MCRegister Dst;
    MemoryOperand Mem;
    if (parseGPR(Dst) || Parser.parseComma() || parseMemory(Mem))
      return true;
    if (Mem.Kind != MemoryKind::Program || Mem.PostIncrement)
      return error(NameLoc, "expected program operand '[pb:rN]' or '[pb:rN+disp]'");
    MCInst Inst;
    Inst.setOpcode(Mem.Disp ? (IsWord ? AVM::LDP16_DISP : AVM::LDP8_DISP)
                            : (IsWord ? AVM::LDP16 : AVM::LDP8));
    Inst.addOperand(MCOperand::createReg(Dst));
    Inst.addOperand(MCOperand::createReg(Mem.Base));
    if (Mem.Disp)
      addExpr(Inst, Mem.Disp);
    return finishInstruction(std::move(Inst), Parser.getTok().getLoc(), Operands,
                             Name, NameLoc);
  }

public:
  AVMAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
               const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII), Parser(Parser) {
    MCAsmParserExtension::Initialize(Parser);
  }

  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                     SMLoc &EndLoc) override {
    return parseGPR(Reg, &StartLoc, &EndLoc);
  }

  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override {
    if (!Parser.getTok().is(AsmToken::Identifier))
      return ParseStatus::NoMatch;
    StringRef Name = Parser.getTok().getIdentifier();
    if (!Name.starts_with_insensitive("r") &&
        !Name.starts_with_insensitive("c") &&
        !Name.starts_with_insensitive("b"))
      return ParseStatus::NoMatch;
    if (parseGPR(Reg, &StartLoc, &EndLoc))
      return ParseStatus::Failure;
    return ParseStatus::Success;
  }

  void convertToMapAndConstraints(unsigned,
                                  const OperandVector &) override {}

  ParseStatus parseDirective(AsmToken) override {
    return ParseStatus::NoMatch;
  }

  bool parseInstruction(ParseInstructionInfo &, StringRef Name, SMLoc NameLoc,
                        OperandVector &Operands) override {
    Pending.reset();
    std::string LowerStorage = Name.lower();
    StringRef M = LowerStorage;

    if (M == "ret") return parseNoOperand(AVM::RET, Name, NameLoc, Operands);
    if (M == "nop") return parseNoOperand(AVM::NOP, Name, NameLoc, Operands);
    if (M == "clr") return parseOneReg(AVM::CLR, Name, NameLoc, Operands, true);
    if (M == "tst16") return parseOneReg(AVM::TST16, Name, NameLoc, Operands);
    if (M == "tst8") return parseOneReg(AVM::TST8, Name, NameLoc, Operands);

    if (M == "mov") return parseRegReg(AVM::MOVC, AVM::MOV16, Name, NameLoc, Operands, true);
    if (M == "mov16") return parseRegReg(AVM::MOV16_E3, AVM::MOV16_E3, Name, NameLoc, Operands);
    if (M == "mov8z") return parseRegReg(AVM::MOV8Z, AVM::MOV8Z, Name, NameLoc, Operands);
    if (M == "mov8s") return parseRegReg(AVM::MOV8S, AVM::MOV8S, Name, NameLoc, Operands);
    if (M == "cset") return parseCSet(Name, NameLoc, Operands);
    if (M == "add") return parseRegReg(AVM::ADDC, AVM::ADD16, Name, NameLoc, Operands);
    if (M == "sub") return parseRegReg(AVM::SUBC, AVM::SUB16, Name, NameLoc, Operands);
    if (M == "add.nf") return parseRegReg(AVM::ADDNF, AVM::ADDNF, Name, NameLoc, Operands);
    if (M == "sub.nf") return parseRegReg(AVM::SUBNF, AVM::SUBNF, Name, NameLoc, Operands);
    if (M == "cmp16") return parseRegReg(AVM::CMP16C, AVM::CMP16, Name, NameLoc, Operands, true);
    if (M == "cmp8") return parseRegReg(AVM::CMP8C, AVM::CMP8, Name, NameLoc, Operands, true);

    if (M == "ld8") return parseLoadStore(Name, NameLoc, Operands, true, false, false);
    if (M == "st8") return parseLoadStore(Name, NameLoc, Operands, false, false, false);
    if (M == "ld16") return parseLoadStore(Name, NameLoc, Operands, true, true, false);
    if (M == "st16") return parseLoadStore(Name, NameLoc, Operands, false, true, false);
    if (M == "ld8_post") return parseLoadStore(Name, NameLoc, Operands, true, false, true);
    if (M == "st8_post") return parseLoadStore(Name, NameLoc, Operands, false, false, true);
    if (M == "ld16_post") return parseLoadStore(Name, NameLoc, Operands, true, true, true);
    if (M == "st16_post") return parseLoadStore(Name, NameLoc, Operands, false, true, true);

    if (M == "ldsp8") return parseStack(Name, NameLoc, Operands, true, false);
    if (M == "stsp8") return parseStack(Name, NameLoc, Operands, false, false);
    if (M == "ldsp16") return parseStack(Name, NameLoc, Operands, true, true);
    if (M == "stsp16") return parseStack(Name, NameLoc, Operands, false, true);
    if (M == "ldm8") return parseDirectMemory(Name, NameLoc, Operands, true, false);
    if (M == "stm8") return parseDirectMemory(Name, NameLoc, Operands, false, false);
    if (M == "ldm16") return parseDirectMemory(Name, NameLoc, Operands, true, true);
    if (M == "stm16") return parseDirectMemory(Name, NameLoc, Operands, false, true);
    if (M == "ldp8") return parseProgramLoad(Name, NameLoc, Operands, false);
    if (M == "ldp16") return parseProgramLoad(Name, NameLoc, Operands, true);

    if (M == "lea") {
      MCRegister Dst;
      MemoryOperand Mem;
      if (parseGPR(Dst) || Parser.parseComma() || parseMemory(Mem)) return true;
      if (Mem.Kind != MemoryKind::Data || !Mem.Disp || Mem.PostIncrement)
        return error(NameLoc, "LEA requires '[rN+disp]'");
      MCInst Inst; Inst.setOpcode(AVM::LEA);
      Inst.addOperand(MCOperand::createReg(Dst));
      Inst.addOperand(MCOperand::createReg(Mem.Base));
      addExpr(Inst, Mem.Disp);
      return finishInstruction(std::move(Inst), Parser.getTok().getLoc(), Operands, Name, NameLoc);
    }

    if (M == "push16") return parseOneReg(AVM::PUSH16, Name, NameLoc, Operands);
    if (M == "pop16") return parseOneReg(AVM::POP16, Name, NameLoc, Operands);
    if (M == "inc16") return parseOneReg(AVM::INC16, Name, NameLoc, Operands);
    if (M == "dec16") return parseOneReg(AVM::DEC16, Name, NameLoc, Operands);

    unsigned Unary = StringSwitch<unsigned>(M)
      .Case("not16", AVM::NOT16).Case("neg16", AVM::NEG16)
      .Case("lsl16", AVM::LSL16).Case("lsr16", AVM::LSR16)
      .Case("asr16", AVM::ASR16).Case("lsr8", AVM::LSR8)
      .Case("asr8", AVM::ASR8).Case("zext8", AVM::ZEXT8)
      .Case("sext8", AVM::SEXT8).Case("swap8", AVM::SWAP8)
      .Case("getsp", AVM::GETSP).Case("setsp", AVM::SETSP)
      .Case("jmpr", AVM::JMPR).Case("callr", AVM::CALLR)
      .Case("jmpp", AVM::JMPP).Case("callp", AVM::CALLP)
      .Case("mtpb", AVM::MTPB).Case("mfpb", AVM::MFPB)
      .Default(0);
    if (Unary) return parseOneReg(Unary, Name, NameLoc, Operands);

    unsigned Binary = StringSwitch<unsigned>(M)
      .Case("and", AVM::AND16).Case("or", AVM::OR16)
      .Case("xor", AVM::XOR16).Case("bic", AVM::BIC16)
      .Case("mulu8", AVM::MULU8)
      .Case("muls8", AVM::MULS8).Case("mulsu8", AVM::MULSU8)
      .Case("shl16v", AVM::SHL16V).Case("lsr16v", AVM::LSR16V)
      .Case("asr16v", AVM::ASR16V).Default(0);
    if (Binary) return parseRegReg(Binary, Binary, Name, NameLoc, Operands);

    if (M == "ldi8") return parseRegImm(AVM::LDI8, Name, NameLoc, Operands, true, AVM::LDI8C);
    unsigned RegImm = StringSwitch<unsigned>(M)
      .Case("ldi16", AVM::LDI16).Case("addi16", AVM::ADDI16)
      .Case("subi16", AVM::SUBI16).Case("andi16", AVM::ANDI16)
      .Case("ori16", AVM::ORI16).Case("xori16", AVM::XORI16)
      .Case("cmpi16", AVM::CMPI16).Case("cmpi8", AVM::CMPI8)
      .Case("cmpi6", AVM::CMPI6).Default(0);
    if (RegImm) return parseRegImm(RegImm, Name, NameLoc, Operands);

    unsigned Branch = StringSwitch<unsigned>(M)
      .Case("beq.s", AVM::BEQ_SHORT).Case("bne.s", AVM::BNE_SHORT)
      .Case("breq", AVM::BREQ).Case("brne", AVM::BRNE)
      .Case("brult", AVM::BRULT).Case("bruge", AVM::BRUGE)
      .Case("brslt", AVM::BRSLT).Case("brsge", AVM::BRSGE)
      .Case("brule", AVM::BRULE).Case("brugt", AVM::BRUGT)
      .Case("jmp", AVM::JMP_REL8).Case("call", AVM::CALL_REL8)
      .Case("jmp16", AVM::JMP16).Case("call16", AVM::CALL16)
      .Case("jmpf", AVM::JMPF).Case("callf", AVM::CALLF)
      .Case("ldpbi", AVM::LDPBI).Case("sys", AVM::SYS)
      .Case("adjsp", AVM::ADJSP).Default(0);
    if (Branch) return parseBranch(Branch, Name, NameLoc, Operands);

    return error(NameLoc, Twine("unknown AVM instruction '") + Name + "'");
  }

  bool matchAndEmitInstruction(SMLoc Loc, unsigned &, OperandVector &,
                               MCStreamer &Out, uint64_t &,
                               bool) override {
    if (!Pending)
      return error(Loc, "internal AVM parser error: no pending instruction");
    Out.emitInstruction(*Pending, getSTI());
    Pending.reset();
    return false;
  }
};

} // namespace

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVMAsmParser() {
  RegisterMCAsmParser<AVMAsmParser> X(getTheAVMTarget());
}
