#include "AVMFixupKinds.h"
#include "AVMMCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
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
                         bool PCRel = false) const {
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
      // transfers are based on nextPC, one byte beyond that place.
      Expr = MCBinaryExpr::createSub(
          Expr, MCConstantExpr::create(1, Ctx), Ctx);
    }
    Fixups.push_back(MCFixup::create(Offset, Expr, Kind, PCRel));
    return 0;
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
                  bool Signed = false) const {
    emit8(Out, 0xf4);
    emit8(Out, Base | regIndex(MI, MI.getOperand(0).getReg()));
    const MCOperand &MO = MI.getOperand(1);
    if (MO.isExpr()) {
      error(MI, "8-bit instruction immediate must be absolute");
      emit8(Out, 0);
      return;
    }
    int64_t V = getImm(MI, 1, Signed ? -128 : 0, Signed ? 127 : 255,
                       "8-bit immediate");
    emit8(Out, V);
  }

  void emitF4Imm16(const MCInst &MI, SmallVectorImpl<char> &Out,
                   SmallVectorImpl<MCFixup> &Fixups, uint8_t Base) const {
    emit8(Out, 0xf4);
    emit8(Out, Base | regIndex(MI, MI.getOperand(0).getReg()));
    uint64_t V = emitExprOrImm(MI, 1, 2, AVM::fixup_avm_data16, Fixups);
    if (MI.getOperand(1).isImm() && !isUInt<16>(V))
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
    case AVM::CLR: {
      unsigned R = compactIndex(MI, MI.getOperand(0).getReg());
      emit8(Out, (R << 2) | R);
      return;
    }
    case AVM::MOVC: {
      unsigned D = compactIndex(MI, MI.getOperand(0).getReg());
      unsigned S = compactIndex(MI, MI.getOperand(1).getReg());
      if (D == S)
        error(MI, "compact self-move encodes CLR; use the full MOV form");
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
    case AVM::TST16: {
      unsigned R = compactIndex(MI, MI.getOperand(0).getReg());
      emit8(Out, 0xa0 | (R << 2) | R); return;
    }
    case AVM::CMP8C: {
      unsigned D = compactIndex(MI, MI.getOperand(0).getReg());
      unsigned S = compactIndex(MI, MI.getOperand(1).getReg());
      if (D == S)
        error(MI, "compact self-compare encodes TST8; use full CMP8");
      emit8(Out, 0xb0 | (D << 2) | S); return;
    }
    case AVM::TST8: {
      unsigned R = compactIndex(MI, MI.getOperand(0).getReg());
      emit8(Out, 0xb0 | (R << 2) | R); return;
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
      emit8(Out, getImm(MI, 1, 0, 255, "LDI8 immediate"));
      return;
    }
    case AVM::BREQ: case AVM::BRNE: case AVM::BRULT: case AVM::BRUGE:
    case AVM::BRSLT: case AVM::BRSGE: case AVM::BRULE: case AVM::BRUGT: {
      uint8_t Opcode = 0xf5 + (MI.getOpcode() - AVM::BREQ);
      emit8(Out, Opcode);
      uint64_t V = emitExprOrImm(MI, 0, 1, AVM::fixup_avm_pcrel8,
                                 Fixups, true);
      if (MI.getOperand(0).isImm() && !isInt<8>(static_cast<int64_t>(V)))
        error(MI, "branch displacement is out of signed 8-bit range");
      emit8(Out, V);
      return;
    }

    case AVM::NOT16: emitF4Unary(MI, Out, 0x00); return;
    case AVM::NEG16: emitF4Unary(MI, Out, 0x08); return;
    case AVM::LSL16: emitF4Unary(MI, Out, 0x20); return;
    case AVM::LSR16: emitF4Unary(MI, Out, 0x28); return;
    case AVM::ASR16: emitF4Unary(MI, Out, 0x30); return;
    case AVM::LSR8: emitF4Unary(MI, Out, 0x38); return;
    case AVM::ASR8: emitF4Unary(MI, Out, 0x40); return;
    case AVM::ZEXT8: emitF4Unary(MI, Out, 0x48); return;
    case AVM::SEXT8: emitF4Unary(MI, Out, 0x50); return;
    case AVM::SWAP8: emitF4Unary(MI, Out, 0x58); return;
    case AVM::GETSP: emitF4Unary(MI, Out, 0x60); return;
    case AVM::SETSP: emitF4Unary(MI, Out, 0x68); return;

    case AVM::AND16: emitF4Binary(MI, Out, 0x60); return;
    case AVM::OR16: emitF4Binary(MI, Out, 0x61); return;
    case AVM::XOR16: emitF4Binary(MI, Out, 0x62); return;
    case AVM::BIC16: emitF4Binary(MI, Out, 0x63); return;
    case AVM::ADC16: emitF4Binary(MI, Out, 0x64); return;
    case AVM::SBC16: emitF4Binary(MI, Out, 0x65); return;
    case AVM::CMP8: emitF4Binary(MI, Out, 0x66); return;
    case AVM::CPC16: emitF4Binary(MI, Out, 0x67); return;
    case AVM::MULU8: emitF4Binary(MI, Out, 0x68); return;
    case AVM::MULS8: emitF4Binary(MI, Out, 0x69); return;
    case AVM::MULSU8: emitF4Binary(MI, Out, 0x6a); return;
    case AVM::SHL16V: emitF4Binary(MI, Out, 0x6b); return;
    case AVM::LSR16V: emitF4Binary(MI, Out, 0x6c); return;
    case AVM::ASR16V: emitF4Binary(MI, Out, 0x6d); return;

    case AVM::LDI16: emitF4Imm16(MI, Out, Fixups, 0x70); return;
    case AVM::LDI8: emitF4Imm8(MI, Out, Fixups, 0x78); return;
    case AVM::ADDI16: emitF4Imm16(MI, Out, Fixups, 0x80); return;
    case AVM::SUBI16: emitF4Imm16(MI, Out, Fixups, 0x88); return;
    case AVM::ANDI16: emitF4Imm16(MI, Out, Fixups, 0x90); return;
    case AVM::ORI16: emitF4Imm16(MI, Out, Fixups, 0x98); return;
    case AVM::XORI16: emitF4Imm16(MI, Out, Fixups, 0xa0); return;
    case AVM::CMPI16: emitF4Imm16(MI, Out, Fixups, 0xa8); return;
    case AVM::CMPI8: emitF4Imm8(MI, Out, Fixups, 0xb0); return;

    case AVM::MOV16: emitF4Binary(MI, Out, 0xb8); return;
    case AVM::ADD16: emitF4Binary(MI, Out, 0xb9); return;
    case AVM::SUB16: emitF4Binary(MI, Out, 0xba); return;
    case AVM::CMP16: emitF4Binary(MI, Out, 0xbb); return;
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

    case AVM::JMPR: emitF4Unary(MI, Out, 0xc0); return;
    case AVM::CALLR: emitF4Unary(MI, Out, 0xc8); return;
    case AVM::JMPP: emitF4Unary(MI, Out, 0xd0); return;
    case AVM::CALLP: emitF4Unary(MI, Out, 0xd8); return;
    case AVM::MTPB: emitF4Unary(MI, Out, 0xe0); return;
    case AVM::MFPB: emitF4Unary(MI, Out, 0xe8); return;
    case AVM::LDPBI:
      emit8(Out, 0xe8);
      emit8(Out, getImm(MI, 0, 0, 255, "PB immediate")); return;
    case AVM::JMP16:
    case AVM::CALL16: {
      emit8(Out, MI.getOpcode() == AVM::JMP16 ? 0xea : 0xeb);
      uint64_t V = emitExprOrImm(MI, 0, 1, AVM::fixup_avm_bank16, Fixups);
      if (MI.getOperand(0).isImm() && !isUInt<16>(V))
        error(MI, "same-bank absolute target is out of range");
      emit16(Out, V); return;
    }
    case AVM::NOP:
      emit8(Out, 0xec); return;
    case AVM::SYS:
      emit8(Out, 0xe9);
      emit8(Out, getImm(MI, 0, 0, 255, "SYS service")); return;

    case AVM::LD8: emitFDReg(MI, Out, 0x00, 0, 1); return;
    case AVM::ST8: emitFDReg(MI, Out, 0x01, 1, 0); return;
    case AVM::LD16: emitFDReg(MI, Out, 0x02, 0, 1); return;
    case AVM::ST16: emitFDReg(MI, Out, 0x03, 1, 0); return;
    case AVM::LD8_POST:
      if (MI.getOperand(0).getReg() == MI.getOperand(1).getReg())
        error(MI, "postincrement load destination must differ from address");
      emitFDReg(MI, Out, 0x04, 0, 1); return;
    case AVM::ST8_POST: emitFDReg(MI, Out, 0x05, 1, 0); return;
    case AVM::LD16_POST:
      if (MI.getOperand(0).getReg() == MI.getOperand(1).getReg())
        error(MI, "postincrement load destination must differ from address");
      emitFDReg(MI, Out, 0x06, 0, 1); return;
    case AVM::ST16_POST: emitFDReg(MI, Out, 0x07, 1, 0); return;
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
      emit8(Out, rrSpec(MI, 2, 0));
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
      emit8(Out, 0xfe);
      uint64_t V = emitExprOrImm(MI, 0, 1, AVM::fixup_avm_far24, Fixups);
      if (MI.getOperand(0).isImm()) {
        if (!isUInt<24>(V)) error(MI, "far target is out of 24-bit range");
        if (V & 1) error(MI, "far target must be even-aligned");
      }
      V = (V & 0xfffffe) | (MI.getOpcode() == AVM::CALLF ? 1 : 0);
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
