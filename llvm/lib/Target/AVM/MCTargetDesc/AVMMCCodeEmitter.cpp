#include "AVMFixupKinds.h"
#include "AVMMCExpr.h"
#include "AVMMCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/MathExtras.h"
#include <cstdint>

using namespace llvm;

namespace {

class AVMMCCodeEmitter final : public MCCodeEmitter {
  MCContext &Ctx;

  void error(const MCInst &MI, const Twine &Message) const {
    Ctx.reportError(MI.getLoc(), Message);
  }

  static void emit8(SmallVectorImpl<char> &Out, uint64_t Value) {
    Out.push_back(static_cast<char>(Value & 0xff));
  }
  static void emit16(SmallVectorImpl<char> &Out, uint64_t Value) {
    emit8(Out, Value);
    emit8(Out, Value >> 8);
  }
  static void emit24(SmallVectorImpl<char> &Out, uint64_t Value) {
    emit8(Out, Value);
    emit8(Out, Value >> 8);
    emit8(Out, Value >> 16);
  }

  unsigned regIndex(const MCInst &MI, MCRegister Reg) const {
    switch (Reg.id()) {
    case AVM::R0: case AVM::B0: return 0;
    case AVM::R1: case AVM::B1: return 1;
    case AVM::R2: case AVM::B2: return 2;
    case AVM::R3: case AVM::B3: return 3;
    case AVM::R4: case AVM::B4: return 4;
    case AVM::R5: case AVM::B5: return 5;
    case AVM::R6: case AVM::B6: return 6;
    case AVM::R7: case AVM::B7: return 7;
    default:
      error(MI, "expected AVM general-purpose register");
      return 0;
    }
  }

  unsigned compactIndex(const MCInst &MI, MCRegister Reg) const {
    unsigned R = regIndex(MI, Reg);
    if (R < 4) {
      error(MI, "instruction requires compact register c0-c3 (r4-r7)");
      return 0;
    }
    return R - 4;
  }

  unsigned pairIndex(const MCInst &MI, MCRegister Reg) const {
    switch (Reg.id()) {
    case AVM::R0R1: return 0;
    case AVM::R2R3: return 1;
    case AVM::R4R5: return 2;
    case AVM::R6R7: return 3;
    default:
      error(MI, "instruction requires aligned register pair q0-q3");
      return 0;
    }
  }

  uint8_t rrSpec(const MCInst &MI, unsigned DstOp, unsigned SrcOp) const {
    unsigned D = regIndex(MI, MI.getOperand(DstOp).getReg());
    unsigned S = regIndex(MI, MI.getOperand(SrcOp).getReg());
    // Nibbles are pre-scaled by two so the interpreter can form the native
    // AVR low-byte address as 8+nibble without executing LSL.
    return static_cast<uint8_t>((D << 5) | (S << 1));
  }

  int64_t getImm(const MCInst &MI, unsigned Op, int64_t Min, int64_t Max,
                 StringRef What) const {
    const MCOperand &MO = MI.getOperand(Op);
    if (!MO.isImm()) {
      error(MI, Twine(What) + " must be an absolute immediate");
      return 0;
    }
    int64_t Value = MO.getImm();
    if (Value < Min || Value > Max)
      error(MI, Twine(What) + " is out of range");
    return Value;
  }

  uint64_t emitExprOrImm(const MCInst &MI, unsigned Op, unsigned Offset,
                         AVM::Fixups Kind, SmallVectorImpl<MCFixup> &Fixups,
                         bool PCRel = false, unsigned NextPCBias = 1) const {
    const MCOperand &MO = MI.getOperand(Op);
    if (MO.isImm())
      return static_cast<uint64_t>(MO.getImm());
    if (!MO.isExpr()) {
      error(MI, "expected immediate or expression");
      return 0;
    }
    const MCExpr *Expr = MO.getExpr();
    if (PCRel) {
      // Relocation place is the final displacement byte, while AVM relative
      // transfers are based on nextPC. The relocation place is the first
      // displacement byte, so exact rel8 and rel16 forms need different
      // addends to make P identify the primary opcode.
      Expr = MCBinaryExpr::createSub(
          Expr, MCConstantExpr::create(NextPCBias, Ctx), Ctx);
    }
    Fixups.push_back(MCFixup::create(Offset, Expr, Kind, PCRel));
    return 0;
  }

  const AVMMCExpr *getTargetExpr(const MCInst &MI, unsigned Op,
                                 StringRef What) const {
    const MCOperand &MO = MI.getOperand(Op);
    if (!MO.isExpr())
      return nullptr;
    const auto *Expr = dyn_cast<AVMMCExpr>(MO.getExpr());
    if (!Expr)
      error(MI, Twine("symbolic ") + What +
                    " requires an AVM address modifier");
    return Expr;
  }

  bool rejectTargetExpr(const MCInst &MI, unsigned Op, StringRef What) const {
    if (!MI.getOperand(Op).isExpr() ||
        !isa<AVMMCExpr>(MI.getOperand(Op).getExpr()))
      return false;
    error(MI, Twine("AVM address modifiers are not valid for ") + What);
    return true;
  }

  uint64_t emitProgHi8(const MCInst &MI, unsigned Op, unsigned Offset,
                       SmallVectorImpl<MCFixup> &Fixups,
                       StringRef What) const {
    const MCOperand &MO = MI.getOperand(Op);
    if (MO.isImm()) {
      int64_t Value = MO.getImm();
      if (!isUInt<8>(Value))
        error(MI, Twine(What) + " is out of unsigned 8-bit range");
      return static_cast<uint64_t>(Value);
    }

    const AVMMCExpr *Expr = getTargetExpr(MI, Op, What);
    if (!Expr)
      return 0;
    if (Expr->getVariantKind() != AVMMCExpr::VK_ProgHi8) {
      error(MI, Twine(What) + " requires prog_hi8(expression), not " +
                    Expr->getVariantName());
      return 0;
    }

    int64_t Absolute;
    if (Expr->getSubExpr()->evaluateAsAbsolute(Absolute)) {
      if (!isUInt<24>(Absolute))
        error(MI, "program address is out of unsigned 24-bit range");
      return static_cast<uint64_t>(Absolute) >> 16;
    }
    return emitExprOrImm(MI, Op, Offset, AVM::fixup_avm_prog_hi8, Fixups);
  }

  void emitCompactBinary(const MCInst &MI, SmallVectorImpl<char> &Out,
                         uint8_t Base) const {
    unsigned D = compactIndex(MI, MI.getOperand(0).getReg());
    unsigned S = compactIndex(MI, MI.getOperand(1).getReg());
    emit8(Out, Base | (D << 2) | S);
  }

  void emitF4Unary(const MCInst &MI, SmallVectorImpl<char> &Out,
                   uint8_t Base) const {
    emit8(Out, 0xe0);
    emit8(Out, Base | regIndex(MI, MI.getOperand(0).getReg()));
  }

  void emitF4Binary(const MCInst &MI, SmallVectorImpl<char> &Out,
                    uint8_t Secondary) const {
    emit8(Out, 0xf4);
    emit8(Out, Secondary);
    emit8(Out, rrSpec(MI, 0, 1));
  }

  void emitF4Imm8(const MCInst &MI, SmallVectorImpl<char> &Out,
                  SmallVectorImpl<MCFixup> &Fixups, uint8_t Base,
                  StringRef What) const {
    emit8(Out, 0xe0);
    emit8(Out, Base | regIndex(MI, MI.getOperand(0).getReg()));
    emit8(Out, emitProgHi8(MI, 1, 2, Fixups, What));
  }

  void emitF4Imm16(const MCInst &MI, SmallVectorImpl<char> &Out,
                   SmallVectorImpl<MCFixup> &Fixups, uint8_t Base) const {
    emit8(Out, 0xe0);
    emit8(Out, Base | regIndex(MI, MI.getOperand(0).getReg()));
    AVM::Fixups Kind = AVM::fixup_avm_data16;
    if (MI.getOperand(1).isExpr()) {
      if (const auto *Expr = dyn_cast<AVMMCExpr>(MI.getOperand(1).getExpr())) {
        if (Expr->getVariantKind() != AVMMCExpr::VK_ProgLo16) {
          error(MI, "16-bit instruction immediate requires "
                    "prog_lo16(expression), not prog_hi8(expression)");
          emit16(Out, 0);
          return;
        }
        int64_t Absolute;
        if (Expr->getSubExpr()->evaluateAsAbsolute(Absolute)) {
          if (!isUInt<24>(Absolute))
            error(MI, "program address is out of unsigned 24-bit range");
          emit16(Out, static_cast<uint64_t>(Absolute) & 0xffff);
          return;
        }
        Kind = AVM::fixup_avm_prog_lo16;
      }
    }
    uint64_t V = emitExprOrImm(MI, 1, 2, Kind, Fixups);
    if (MI.getOperand(1).isImm() &&
        !isUInt<16>(V) && !isInt<16>(MI.getOperand(1).getImm()))
      error(MI, "16-bit immediate is out of range");
    emit16(Out, V);
  }

  void emitFDReg(const MCInst &MI, SmallVectorImpl<char> &Out,
                 uint8_t Secondary, unsigned DstOp, unsigned SrcOp) const {
    emit8(Out, 0xfd);
    emit8(Out, Secondary);
    emit8(Out, rrSpec(MI, DstOp, SrcOp));
  }

public:
  explicit AVMMCCodeEmitter(MCContext &Ctx) : Ctx(Ctx) {}

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &Out,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &) const override {
    switch (MI.getOpcode()) {
    case AVM::MOV32:
    case AVM::ADD32:
    case AVM::SUB32:
    case AVM::AND32:
    case AVM::OR32:
    case AVM::XOR32:
    case AVM::CMP32:
    case AVM::SHL32V:
    case AVM::LSR32V:
    case AVM::ASR32V: {
      unsigned Op = MI.getOpcode() == AVM::MOV32 ? 0
                    : MI.getOpcode() == AVM::ADD32 ? 1
                    : MI.getOpcode() == AVM::SUB32 ? 2
                    : MI.getOpcode() == AVM::AND32 ? 3
                    : MI.getOpcode() == AVM::OR32 ? 4
                    : MI.getOpcode() == AVM::XOR32 ? 5
                    : MI.getOpcode() == AVM::CMP32 ? 6
                    : MI.getOpcode() == AVM::SHL32V ? 7
                    : MI.getOpcode() == AVM::LSR32V ? 8 : 9;
      unsigned D = pairIndex(MI, MI.getOperand(0).getReg());
      unsigned S = pairIndex(MI, MI.getOperand(1).getReg());
      emit8(Out, 0xe1);
      emit8(Out, (Op << 4) | (D << 2) | S);
      return;
    }
    case AVM::CLR: {
      unsigned R = compactIndex(MI, MI.getOperand(0).getReg());
      emit8(Out, (R << 2) | R);
      return;
    }
    case AVM::MOVC: {
      unsigned D = compactIndex(MI, MI.getOperand(0).getReg());
      unsigned S = compactIndex(MI, MI.getOperand(1).getReg());
      if (D == S) {
        error(MI, "compact self-move has no encoding; use NOP");
        return;
      }
      emit8(Out, (D << 2) | S);
      return;
    }
    case AVM::LD8C: emitCompactBinary(MI, Out, 0x10); return;
    case AVM::ST8C: emitCompactBinary(MI, Out, 0x20); return;
    case AVM::LD16C: emitCompactBinary(MI, Out, 0x30); return;
    case AVM::ST16C: emitCompactBinary(MI, Out, 0x40); return;
    case AVM::PUSH16:
      emit8(Out, 0x70 | regIndex(MI, MI.getOperand(0).getReg())); return;
    case AVM::POP16:
      emit8(Out, 0x78 | regIndex(MI, MI.getOperand(0).getReg())); return;
    case AVM::ADDC: emitCompactBinary(MI, Out, 0x80); return;
    case AVM::SUBC: emitCompactBinary(MI, Out, 0x90); return;
    case AVM::CMP16C: {
      unsigned D = compactIndex(MI, MI.getOperand(0).getReg());
      unsigned S = compactIndex(MI, MI.getOperand(1).getReg());
      if (D == S)
        error(MI, "compact self-compare encodes TST16; use full CMP16");
      emit8(Out, 0xa0 | (D << 2) | S); return;
    }
    case AVM::TST16C: {
      unsigned C = compactIndex(MI, MI.getOperand(0).getReg());
      emit8(Out, 0xa0 | (C << 2) | C);
      return;
    }
    case AVM::TST16: {
      unsigned R = regIndex(MI, MI.getOperand(0).getReg());
      if (R >= 4) {
        error(MI, "E0 TST16 requires r0-r3");
        return;
      }
      emit8(Out, 0xe0); emit8(Out, 0xe8 | R);
      return;
    }
    case AVM::CMP8C: {
      unsigned D = compactIndex(MI, MI.getOperand(0).getReg());
      unsigned S = compactIndex(MI, MI.getOperand(1).getReg());
      if (D == S)
        error(MI, "compact self-compare encodes TST8; use full CMP8");
      emit8(Out, 0xb0 | (D << 2) | S); return;
    }
    case AVM::TST8C: {
      unsigned C = compactIndex(MI, MI.getOperand(0).getReg());
      emit8(Out, 0xb0 | (C << 2) | C);
      return;
    }
    case AVM::TST8: {
      unsigned R = regIndex(MI, MI.getOperand(0).getReg());
      if (R >= 4) {
        error(MI, "E0 TST8 requires r0-r3");
        return;
      }
      emit8(Out, 0xe0); emit8(Out, 0xf0 | R);
      return;
    }
    case AVM::BEQ_SHORT:
    case AVM::BNE_SHORT: {
      int64_t D = getImm(MI, 0, -9, 8, "short branch displacement");
      if (D == -1 || D == 0)
        error(MI, "short branch displacement must be -9..-2 or +1..+8");
      unsigned Nibble = D < 0 ? static_cast<unsigned>(D + 9)
                              : static_cast<unsigned>(D + 7);
      emit8(Out, (MI.getOpcode() == AVM::BEQ_SHORT ? 0xc0 : 0xd0) |
                     (Nibble & 0xf));
      return;
    }
    case AVM::INC16:
      emitF4Unary(MI, Out, 0x10); return;
    case AVM::DEC16:
      emitF4Unary(MI, Out, 0x18); return;
    case AVM::LDI8C: {
      emit8(Out, 0xf0 | compactIndex(MI, MI.getOperand(0).getReg()));
      emit8(Out, emitProgHi8(MI, 1, 1, Fixups, "LDI8 immediate"));
      return;
    }
    case AVM::ADDNF:
    case AVM::SUBNF: {
      unsigned FullD = regIndex(MI, MI.getOperand(0).getReg());
      unsigned FullS = regIndex(MI, MI.getOperand(1).getReg());
      if (FullD == 4 && FullS < 4) {
        emit8(Out, 0xe2);
        emit8(Out, (MI.getOpcode() == AVM::ADDNF ? 0x04 : 0x08) | FullS);
        return;
      }
      unsigned D = compactIndex(MI, MI.getOperand(0).getReg());
      unsigned S = compactIndex(MI, MI.getOperand(1).getReg());
      if (MI.getOpcode() == AVM::SUBNF && D == S) {
        error(MI, "diagonal F4 SUB.NF is reserved; use CLR cN");
        return;
      }
      emit8(Out, 0xf4);
      emit8(Out, ((MI.getOpcode() == AVM::ADDNF ? 1 : 2) << 4) | (D << 2) | S);
      return;
    }
    case AVM::MOV16_E3:
    case AVM::MOV8Z:
    case AVM::MOV8S: {
      unsigned D = regIndex(MI, MI.getOperand(0).getReg());
      unsigned S = regIndex(MI, MI.getOperand(1).getReg());
      unsigned Kind = MI.getOpcode() == AVM::MOV16_E3 ? 0
                      : MI.getOpcode() == AVM::MOV8Z ? 1 : 2;
      emit8(Out, 0xe3);
      emit8(Out, (Kind << 6) | (D << 3) | S);
      return;
    }
    case AVM::CSET: {
      unsigned D = regIndex(MI, MI.getOperand(0).getReg());
      unsigned CC = getImm(MI, 1, 0, 7, "CSET condition code");
      emit8(Out, 0xe3);
      emit8(Out, 0xc0 | (CC << 3) | D);
      return;
    }
    case AVM::BREQ: case AVM::BRNE: case AVM::BRULT: case AVM::BRUGE:
    case AVM::BRSLT: case AVM::BRSGE: case AVM::BRULE: case AVM::BRUGT: {
      uint8_t Opcode;
      switch (MI.getOpcode()) {
      case AVM::BREQ:  Opcode = 0xf5; break;
      case AVM::BRNE:  Opcode = 0xf6; break;
      case AVM::BRULT: Opcode = 0xf7; break;
      case AVM::BRUGE: Opcode = 0xf8; break;
      case AVM::BRSLT: Opcode = 0xf9; break;
      case AVM::BRSGE: Opcode = 0xfa; break;
      case AVM::BRULE: Opcode = 0xfb; break;
      case AVM::BRUGT: Opcode = 0xfc; break;
      default: llvm_unreachable("unexpected AVM branch opcode");
      }
      emit8(Out, Opcode);
      if (rejectTargetExpr(MI, 0, "PC-relative branch target")) {
        emit8(Out, 0);
        return;
      }
      uint64_t V = emitExprOrImm(MI, 0, 1, AVM::fixup_avm_pcrel8,
                                 Fixups, true);
      if (MI.getOperand(0).isImm() && !isInt<8>(static_cast<int64_t>(V)))
        error(MI, "branch displacement is out of signed 8-bit range");
      emit8(Out, V);
      return;
    }

    case AVM::NOT16: emitF4Unary(MI, Out, 0x00); return;
    case AVM::NEG16: emitF4Unary(MI, Out, 0x08); return;
    case AVM::LSL16: {
      unsigned R = regIndex(MI, MI.getOperand(0).getReg());
      if (R >= 4) {
        error(MI, "E0 LSL16 requires r0-r3; use ADD.NF cN,cN");
        return;
      }
      emit8(Out, 0xe0); emit8(Out, 0x20 | R); return;
    }
    case AVM::LSR16: emitF4Unary(MI, Out, 0x28); return;
    case AVM::ASR16: emitF4Unary(MI, Out, 0x30); return;
    case AVM::LSR8: emitF4Unary(MI, Out, 0x38); return;
    case AVM::ASR8: emitF4Unary(MI, Out, 0x40); return;
    case AVM::SWAP8: emitF4Unary(MI, Out, 0x58); return;
    case AVM::GETSP: emitF4Unary(MI, Out, 0x60); return;
    case AVM::SETSP: emitF4Unary(MI, Out, 0x68); return;

    case AVM::ANDA:
    case AVM::ORA:
    case AVM::XORA:
    case AVM::BICA: {
      if (MI.getOperand(0).getReg() != AVM::R4) {
        error(MI, "accumulator logical operation requires destination c0/A");
        return;
      }
      unsigned S = regIndex(MI, MI.getOperand(1).getReg());
      if (S == 4) {
        error(MI, "primary accumulator self-logical encoding is reserved");
        return;
      }
      uint8_t Base = MI.getOpcode() == AVM::ANDA ? 0x50
                     : MI.getOpcode() == AVM::ORA ? 0x58
                     : MI.getOpcode() == AVM::XORA ? 0x60 : 0x68;
      emit8(Out, Base | S); return;
    }
    case AVM::AND16:
    case AVM::OR16:
    case AVM::XOR16:
    case AVM::BIC16: {
      unsigned CD = compactIndex(MI, MI.getOperand(0).getReg());
      unsigned CS = compactIndex(MI, MI.getOperand(1).getReg());
      if (CD == 0) {
        error(MI, "compact logical operation reserves destination c0/A");
        return;
      }
      if (CD == CS) {
        error(MI, "compact logical self-operation encoding is reserved");
        return;
      }
      uint8_t Op = MI.getOpcode() == AVM::AND16 ? 3
                   : MI.getOpcode() == AVM::OR16 ? 4
                   : MI.getOpcode() == AVM::XOR16 ? 5 : 6;
      emit8(Out, 0xf4); emit8(Out, (Op << 4) | (CD << 2) | CS); return;
    }
    case AVM::ADC16: emitF4Binary(MI, Out, 0x64); return;
    case AVM::SBC16: emitF4Binary(MI, Out, 0x65); return;
    case AVM::CPC16: emitF4Binary(MI, Out, 0x67); return;
    case AVM::MULU8: case AVM::MULS8: case AVM::MULSU8:
    case AVM::SHL16V: case AVM::LSR16V: case AVM::ASR16V: {
      unsigned D = regIndex(MI, MI.getOperand(0).getReg());
      unsigned S = regIndex(MI, MI.getOperand(1).getReg());
      unsigned Op = MI.getOpcode() == AVM::MULU8 ? 9
                    : MI.getOpcode() == AVM::MULS8 ? 10
                    : MI.getOpcode() == AVM::MULSU8 ? 11
                    : MI.getOpcode() == AVM::SHL16V ? 12
                    : MI.getOpcode() == AVM::LSR16V ? 13 : 14;
      if (D == 4 && S < 4) {
        static const uint8_t Bases[] = {0x14, 0x18, 0x1c, 0x20, 0x24, 0x28};
        emit8(Out, 0xe2); emit8(Out, Bases[Op - 9] | S); return;
      }
      unsigned CD = compactIndex(MI, MI.getOperand(0).getReg());
      unsigned CS = compactIndex(MI, MI.getOperand(1).getReg());
      emit8(Out, 0xf4); emit8(Out, (Op << 4) | (CD << 2) | CS); return;
    }

    case AVM::LDI16: emitF4Imm16(MI, Out, Fixups, 0x80); return;
    case AVM::LDI8: {
      if (regIndex(MI, MI.getOperand(0).getReg()) >= 4) {
        error(MI, "E0 LDI8 requires r0-r3; use the compact primary form");
        return;
      }
      emitF4Imm8(MI, Out, Fixups, 0x88, "LDI8 immediate"); return;
    }
    case AVM::ADDI16: emitF4Imm16(MI, Out, Fixups, 0x90); return;
    case AVM::SUBI16: emitF4Imm16(MI, Out, Fixups, 0x98); return;
    case AVM::ANDI16: emitF4Imm16(MI, Out, Fixups, 0xa0); return;
    case AVM::ORI16: emitF4Imm16(MI, Out, Fixups, 0xa8); return;
    case AVM::XORI16: emitF4Imm16(MI, Out, Fixups, 0xb0); return;
    case AVM::CMPI16: emitF4Imm16(MI, Out, Fixups, 0xb8); return;
    case AVM::CMPI8:
      emitF4Imm8(MI, Out, Fixups, 0xc0, "CMPI8 immediate"); return;

    case AVM::ADD16: emitF4Binary(MI, Out, 0xb9); return;
    case AVM::SUB16: emitF4Binary(MI, Out, 0xba); return;
    case AVM::CMP16:
    case AVM::CMP8: {
      unsigned D = regIndex(MI, MI.getOperand(0).getReg());
      unsigned S = regIndex(MI, MI.getOperand(1).getReg());
      if (D == 4 && S < 4) {
        emit8(Out, 0xe2);
        emit8(Out, (MI.getOpcode() == AVM::CMP16 ? 0x0c : 0x10) | S);
        return;
      }
      error(MI, "comparison requires compact operands or A,r0-r3"); return;
    }
    case AVM::CMPI6: {
      emit8(Out, 0xe4);
      unsigned R = compactIndex(MI, MI.getOperand(0).getReg());
      int64_t V = getImm(MI, 1, -32, 31, "CMPI6 immediate");
      emit8(Out, ((static_cast<uint8_t>(V) & 0x3f) << 2) | R);
      return;
    }
    case AVM::JMP_REL8:
    case AVM::CALL_REL8: {
      emit8(Out, MI.getOpcode() == AVM::JMP_REL8 ? 0xe5 : 0xe6);
      if (rejectTargetExpr(MI, 0, "PC-relative control target")) {
        emit8(Out, 0);
        return;
      }
      uint64_t V = emitExprOrImm(MI, 0, 1, AVM::fixup_avm_pcrel8,
                                 Fixups, true);
      if (MI.getOperand(0).isImm() && !isInt<8>(static_cast<int64_t>(V)))
        error(MI, "relative control displacement is out of range");
      emit8(Out, V); return;
    }
    case AVM::ADJSP:
      emit8(Out, 0xe7);
      emit8(Out, getImm(MI, 0, -128, 127, "ADJSP immediate")); return;

    case AVM::LDSP8C: case AVM::LDSP16C: {
      emit8(Out, 0xf4); emit8(Out, 0xf9);
      unsigned R = compactIndex(MI, MI.getOperand(0).getReg());
      unsigned Off = getImm(MI, 1, 0, 31, "compact stack offset");
      emit8(Out, (MI.getOpcode() == AVM::LDSP16C ? 0x80 : 0) |
                     (R << 5) | Off);
      return;
    }
    case AVM::STSP8C: case AVM::STSP16C: {
      emit8(Out, 0xf4); emit8(Out, 0xfa);
      unsigned Off = getImm(MI, 0, 0, 31, "compact stack offset");
      unsigned R = compactIndex(MI, MI.getOperand(1).getReg());
      emit8(Out, (MI.getOpcode() == AVM::STSP16C ? 0x80 : 0) |
                     (R << 5) | Off);
      return;
    }

    case AVM::JMPR: emitF4Unary(MI, Out, 0xc8); return;
    case AVM::CALLR: emitF4Unary(MI, Out, 0xd0); return;
    case AVM::JMPP:
      emit8(Out, 0xe0);
      emit8(Out, 0xd8 | pairIndex(MI, MI.getOperand(0).getReg()));
      return;
    case AVM::CALLP:
      emit8(Out, 0xe0);
      emit8(Out, 0xe0 | pairIndex(MI, MI.getOperand(0).getReg()));
      return;
    case AVM::MTPB: emitF4Unary(MI, Out, 0x70); return;
    case AVM::MFPB: emitF4Unary(MI, Out, 0x78); return;
    case AVM::LDPBI: {
      emit8(Out, 0xe8);
      emit8(Out, emitProgHi8(MI, 0, 1, Fixups, "LDPBI immediate"));
      return;
    }
    case AVM::JMP16:
    case AVM::CALL16: {
      emit8(Out, MI.getOpcode() == AVM::JMP16 ? 0xea : 0xeb);
      if (rejectTargetExpr(MI, 0, "PC-relative control target")) {
        emit16(Out, 0);
        return;
      }
      uint64_t V = emitExprOrImm(MI, 0, 1, AVM::fixup_avm_pcrel16, Fixups,
                                 /*PCRel=*/true, /*NextPCBias=*/2);
      if (MI.getOperand(0).isImm() && !isInt<16>(static_cast<int64_t>(V)))
        error(MI, "relative control displacement is out of signed 16-bit range");
      emit16(Out, V); return;
    }
    case AVM::NOP:
      emit8(Out, 0xec); return;
    case AVM::SYS:
      emit8(Out, 0xe9);
      emit8(Out, getImm(MI, 0, 0, 255, "SYS service")); return;

    case AVM::LD8: emitFDReg(MI, Out, 0x00, 0, 1); return;
    case AVM::ST8: emitFDReg(MI, Out, 0x01, 0, 1); return;
    case AVM::LD16: emitFDReg(MI, Out, 0x02, 0, 1); return;
    case AVM::ST16: emitFDReg(MI, Out, 0x03, 0, 1); return;
    case AVM::LD8_POST:
      if (MI.getOperand(0).getReg() == MI.getOperand(1).getReg())
        error(MI, "postincrement load destination must differ from address");
      emitFDReg(MI, Out, 0x04, 0, 1); return;
    case AVM::ST8_POST: emitFDReg(MI, Out, 0x05, 0, 1); return;
    case AVM::LD16_POST:
      if (MI.getOperand(0).getReg() == MI.getOperand(1).getReg())
        error(MI, "postincrement load destination must differ from address");
      emitFDReg(MI, Out, 0x06, 0, 1); return;
    case AVM::ST16_POST: emitFDReg(MI, Out, 0x07, 0, 1); return;
    case AVM::LEA:
    case AVM::LD8_DISP:
    case AVM::LD16_DISP: {
      uint8_t Sec = MI.getOpcode() == AVM::LEA ? 0x08
                    : MI.getOpcode() == AVM::LD8_DISP ? 0x09 : 0x0b;
      emitFDReg(MI, Out, Sec, 0, 1);
      emit8(Out, getImm(MI, 2, -128, 127, "memory displacement"));
      return;
    }
    case AVM::ST8_DISP:
    case AVM::ST16_DISP: {
      uint8_t Sec = MI.getOpcode() == AVM::ST8_DISP ? 0x0a : 0x0c;
      emit8(Out, 0xfd); emit8(Out, Sec);
      emit8(Out, rrSpec(MI, 0, 2));
      emit8(Out, getImm(MI, 1, -128, 127, "memory displacement"));
      return;
    }

    case AVM::LDSP8: case AVM::STSP8:
    case AVM::LDSP16: case AVM::STSP16: {
      bool Store = MI.getOpcode() == AVM::STSP8 ||
                   MI.getOpcode() == AVM::STSP16;
      bool Word = MI.getOpcode() == AVM::LDSP16 ||
                  MI.getOpcode() == AVM::STSP16;
      unsigned RegOp = Store ? 1 : 0;
      unsigned OffOp = Store ? 0 : 1;
      uint8_t Base = Word ? (Store ? 0x28 : 0x20)
                          : (Store ? 0x18 : 0x10);
      emit8(Out, 0xfd);
      emit8(Out, Base | regIndex(MI, MI.getOperand(RegOp).getReg()));
      emit8(Out, getImm(MI, OffOp, 0, 255, "stack offset"));
      return;
    }

    case AVM::LDM8: case AVM::STM8:
    case AVM::LDM16: case AVM::STM16: {
      bool Store = MI.getOpcode() == AVM::STM8 || MI.getOpcode() == AVM::STM16;
      bool Word = MI.getOpcode() == AVM::LDM16 || MI.getOpcode() == AVM::STM16;
      unsigned RegOp = Store ? 1 : 0;
      unsigned AddrOp = Store ? 0 : 1;
      uint8_t Base = Word ? (Store ? 0x48 : 0x40)
                          : (Store ? 0x38 : 0x30);
      emit8(Out, 0xfd);
      emit8(Out, Base | regIndex(MI, MI.getOperand(RegOp).getReg()));
      if (rejectTargetExpr(MI, AddrOp, "direct data-space address")) {
        emit16(Out, 0);
        return;
      }
      uint64_t V = emitExprOrImm(MI, AddrOp, 2, AVM::fixup_avm_data16,
                                 Fixups);
      if (MI.getOperand(AddrOp).isImm() && !isUInt<16>(V))
        error(MI, "direct data-space address is out of range");
      emit16(Out, V);
      return;
    }

    case AVM::LDP8: emitFDReg(MI, Out, 0x80, 0, 1); return;
    case AVM::LDP16: emitFDReg(MI, Out, 0x81, 0, 1); return;
    case AVM::LDP8_DISP:
    case AVM::LDP16_DISP:
      emitFDReg(MI, Out, MI.getOpcode() == AVM::LDP8_DISP ? 0x82 : 0x83,
                0, 1);
      emit8(Out, getImm(MI, 2, -128, 127, "program displacement"));
      return;

    case AVM::JMPF:
    case AVM::CALLF: {
      // The current ISA uses independent primary opcodes, not a link bit in
      // the target address.  In particular, bit zero remains part of target24.
      emit8(Out, MI.getOpcode() == AVM::JMPF ? 0xe2 : 0xe3);
      if (rejectTargetExpr(MI, 0, "far program target")) {
        emit24(Out, 0);
        return;
      }
      uint64_t V = emitExprOrImm(MI, 0, 1, AVM::fixup_avm_far24, Fixups);
      if (MI.getOperand(0).isExpr())
        Fixups.push_back(MCFixup::create(1, MI.getOperand(0).getExpr(),
                                        AVM::fixup_avm_relax));
      if (MI.getOperand(0).isImm()) {
        if (!isUInt<24>(V)) error(MI, "far target is out of 24-bit range");
      }
      emit24(Out, V);
      return;
    }
    case AVM::RET:
      emit8(Out, 0xff); return;
    default:
      error(MI, "unsupported AVM MC opcode");
      return;
    }
  }
};

} // namespace

MCCodeEmitter *llvm::createAVMMCCodeEmitter(const MCInstrInfo &, MCContext &Ctx) {
  return new AVMMCCodeEmitter(Ctx);
}
