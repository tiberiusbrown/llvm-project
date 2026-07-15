#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include <cstdint>

using namespace llvm;

namespace {

class AVMDisassembler final : public MCDisassembler {
  static MCRegister reg(unsigned N) {
    static const MCRegister Regs[] = {AVM::R0, AVM::R1, AVM::R2, AVM::R3,
                                      AVM::R4, AVM::R5, AVM::R6, AVM::R7};
    return Regs[N & 7];
  }
  static MCRegister compact(unsigned N) { return reg(4 + (N & 3)); }
  static MCRegister byteReg(unsigned N) {
    static const MCRegister Regs[] = {AVM::B0, AVM::B1, AVM::B2, AVM::B3,
                                      AVM::B4, AVM::B5, AVM::B6, AVM::B7};
    return Regs[N & 7];
  }
  static MCRegister pair(unsigned N) {
    static const MCRegister Regs[] = {AVM::R0R1, AVM::R2R3,
                                      AVM::R4R5, AVM::R6R7};
    return Regs[N & 3];
  }

  static void addReg(MCInst &MI, MCRegister R) {
    MI.addOperand(MCOperand::createReg(R));
  }
  static void addImm(MCInst &MI, int64_t V) {
    MI.addOperand(MCOperand::createImm(V));
  }

  static bool decodeRR(MCInst &MI, uint8_t RR) {
    if (RR & 0x11)
      return false;
    addReg(MI, reg((RR >> 5) & 7));
    addReg(MI, reg((RR >> 1) & 7));
    return true;
  }

public:
  AVMDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
      : MCDisassembler(STI, Ctx) {}

  DecodeStatus getInstruction(MCInst &MI, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t,
                              raw_ostream &) const override {
    Size = 0;
    if (Bytes.empty())
      return Fail;
    uint8_t Op = Bytes[0];

    if (Op <= 0x0f) {
      unsigned D = (Op >> 2) & 3, S = Op & 3;
      MI.setOpcode(D == S ? AVM::CLR : AVM::MOVC);
      addReg(MI, compact(D));
      if (D != S) addReg(MI, compact(S));
      Size = 1; return Success;
    }
    if (Op <= 0x1f) {
      MI.setOpcode(AVM::LD8C); addReg(MI, compact((Op >> 2) & 3));
      addReg(MI, compact(Op & 3)); Size = 1; return Success;
    }
    if (Op <= 0x2f) {
      MI.setOpcode(AVM::ST8C); addReg(MI, compact((Op >> 2) & 3));
      addReg(MI, compact(Op & 3)); Size = 1; return Success;
    }
    if (Op <= 0x3f) {
      MI.setOpcode(AVM::LD16C); addReg(MI, compact((Op >> 2) & 3));
      addReg(MI, compact(Op & 3)); Size = 1; return Success;
    }
    if (Op <= 0x4f) {
      MI.setOpcode(AVM::ST16C); addReg(MI, compact((Op >> 2) & 3));
      addReg(MI, compact(Op & 3)); Size = 1; return Success;
    }
    if (Op <= 0x57) {
      if (Op == 0x54) { Size = 1; return Fail; }
      MI.setOpcode(AVM::ANDA); addReg(MI, AVM::R4); addReg(MI, reg(Op & 7));
      Size = 1; return Success;
    }
    if (Op <= 0x5f) {
      if (Op == 0x5c) { Size = 1; return Fail; }
      MI.setOpcode(AVM::ORA); addReg(MI, AVM::R4); addReg(MI, reg(Op & 7));
      Size = 1; return Success;
    }
    if (Op <= 0x67) {
      if (Op == 0x64) { Size = 1; return Fail; }
      MI.setOpcode(AVM::XORA); addReg(MI, AVM::R4); addReg(MI, reg(Op & 7));
      Size = 1; return Success;
    }
    if (Op <= 0x6f) {
      if (Op == 0x6c) { Size = 1; return Fail; }
      MI.setOpcode(AVM::BICA); addReg(MI, AVM::R4); addReg(MI, reg(Op & 7));
      Size = 1; return Success;
    }
    if (Op <= 0x77) {
      MI.setOpcode(AVM::PUSH16); addReg(MI, reg(Op & 7)); Size = 1; return Success;
    }
    if (Op <= 0x7f) {
      MI.setOpcode(AVM::POP16); addReg(MI, reg(Op & 7)); Size = 1; return Success;
    }
    if (Op <= 0x8f) {
      MI.setOpcode(AVM::ADDC); addReg(MI, compact((Op >> 2) & 3));
      addReg(MI, compact(Op & 3)); Size = 1; return Success;
    }
    if (Op <= 0x9f) {
      MI.setOpcode(AVM::SUBC); addReg(MI, compact((Op >> 2) & 3));
      addReg(MI, compact(Op & 3)); Size = 1; return Success;
    }
    if (Op <= 0xaf) {
      unsigned D = (Op >> 2) & 3, S = Op & 3;
      MI.setOpcode(D == S ? AVM::TST16C : AVM::CMP16C);
      addReg(MI, compact(D)); if (D != S) addReg(MI, compact(S));
      Size = 1; return Success;
    }
    if (Op <= 0xbf) {
      unsigned D = (Op >> 2) & 3, S = Op & 3;
      MI.setOpcode(D == S ? AVM::TST8C : AVM::CMP8C);
      addReg(MI, compact(D)); if (D != S) addReg(MI, compact(S));
      Size = 1; return Success;
    }
    if (Op <= 0xcf || (Op >= 0xd0 && Op <= 0xdf)) {
      bool Eq = Op < 0xd0;
      unsigned N = Op & 0xf;
      int64_t Disp = N < 8 ? static_cast<int64_t>(N) - 9
                            : static_cast<int64_t>(N) - 7;
      MI.setOpcode(Eq ? AVM::BEQ_SHORT : AVM::BNE_SHORT);
      addImm(MI, Disp); Size = 1; return Success;
    }
    if (Op == 0xe0)
      return decodeE0(MI, Size, Bytes);
    if (Op == 0xe1)
      return decodeE1(MI, Size, Bytes);
    if (Op == 0xe2 || Op == 0xe3) {
      if (Bytes.size() < 4)
        return Fail;
      uint32_t Target = Bytes[1] | uint32_t(Bytes[2]) << 8 |
                        uint32_t(Bytes[3]) << 16;
      MI.setOpcode(Op == 0xe2 ? AVM::JMPF : AVM::CALLF);
      addImm(MI, Target);
      Size = 4;
      return Success;
    }
    if (Op == 0xe4) {
      if (Bytes.size() < 2) return Fail;
      uint8_t X = Bytes[1]; MI.setOpcode(AVM::CMPI6);
      addReg(MI, compact(X & 3)); addImm(MI, static_cast<int8_t>(X) >> 2);
      Size = 2; return Success;
    }
    if (Op >= 0xe5 && Op <= 0xe9) {
      if (Bytes.size() < 2) return Fail;
      unsigned O = Op == 0xe5 ? AVM::JMP_REL8 : Op == 0xe6 ? AVM::CALL_REL8
                   : Op == 0xe7 ? AVM::ADJSP : Op == 0xe8 ? AVM::LDPBI : AVM::SYS;
      MI.setOpcode(O); addImm(MI, Op >= 0xe8 ? Bytes[1] : static_cast<int8_t>(Bytes[1]));
      Size = 2; return Success;
    }
    if (Op == 0xea || Op == 0xeb) {
      if (Bytes.size() < 3) return Fail;
      MI.setOpcode(Op == 0xea ? AVM::JMP16 : AVM::CALL16);
      addImm(MI, Bytes[1] | uint16_t(Bytes[2]) << 8); Size = 3; return Success;
    }
    if (Op == 0xec) { MI.setOpcode(AVM::NOP); Size = 1; return Success; }
    if (Op <= 0xef) { Size = 1; return Fail; }
    if (Op <= 0xf3) {
      if (Bytes.size() < 2) return Fail;
      MI.setOpcode(AVM::LDI8C); addReg(MI, compact(Op & 3)); addImm(MI, Bytes[1]);
      Size = 2; return Success;
    }

    if (Op >= 0xf5 && Op <= 0xfc) {
      if (Bytes.size() < 2) return Fail;
      static const unsigned Opcodes[] = {AVM::BREQ, AVM::BRNE, AVM::BRULT,
          AVM::BRUGE, AVM::BRSLT, AVM::BRSGE, AVM::BRULE, AVM::BRUGT};
      MI.setOpcode(Opcodes[Op - 0xf5]);
      addImm(MI, static_cast<int8_t>(Bytes[1])); Size = 2; return Success;
    }

    if (Op == 0xff) {
      MI.setOpcode(AVM::RET); Size = 1; return Success;
    }
    if (Op == 0xfd)
      return decodeFD(MI, Size, Bytes);
    if (Op == 0xf4)
      return decodeF4(MI, Size, Bytes);

    Size = 1;
    return Fail;
  }

private:
  DecodeStatus decodeE1(MCInst &MI, uint64_t &Size,
                        ArrayRef<uint8_t> B) const {
    if (B.size() < 2)
      return Fail;
    uint8_t S = B[1];
    unsigned Op = S >> 4;
    if (Op >= 10) {
      Size = 2;
      return Fail;
    }
    static const unsigned Ops[] = {
        AVM::MOV32, AVM::ADD32, AVM::SUB32, AVM::AND32, AVM::OR32,
        AVM::XOR32, AVM::CMP32, AVM::SHL32V, AVM::LSR32V, AVM::ASR32V};
    MI.setOpcode(Ops[Op]);
    addReg(MI, pair((S >> 2) & 3));
    addReg(MI, pair(S & 3));
    Size = 2;
    return Success;
  }

  DecodeStatus decodeE3(MCInst &MI, uint64_t &Size,
                        ArrayRef<uint8_t> B) const {
    if (B.size() < 2) return Fail;
    uint8_t S = B[1];
    unsigned Kind = S >> 6;
    if (Kind == 3) {
      MI.setOpcode(AVM::CSET);
      addReg(MI, reg(S & 7));
      addImm(MI, (S >> 3) & 7);
    } else {
      static const unsigned Ops[] = {AVM::MOV16_E3, AVM::MOV8Z, AVM::MOV8S};
      MI.setOpcode(Ops[Kind]);
      addReg(MI, reg((S >> 3) & 7));
      addReg(MI, Kind == 0 ? reg(S & 7) : byteReg(S & 7));
    }
    Size = 2;
    return Success;
  }

  DecodeStatus decodeE2(MCInst &MI, uint64_t &Size,
                        ArrayRef<uint8_t> B) const {
    if (B.size() < 2) return Fail;
    uint8_t S = B[1];
    if (S < 0x04) { Size = 2; return Fail; }
    if (S >= 0x2c) { Size = 2; return Fail; }
    unsigned Group = S >> 2;
    static const unsigned Ops[] = {0, AVM::ADDNF, AVM::SUBNF,
      AVM::CMP16, AVM::CMP8, AVM::MULU8, AVM::MULS8, AVM::MULSU8,
      AVM::SHL16V, AVM::LSR16V, AVM::ASR16V};
    MI.setOpcode(Ops[Group]); addReg(MI, AVM::R4); addReg(MI, reg(S & 3));
    Size = 2; return Success;
  }

  DecodeStatus decodeE0(MCInst &MI, uint64_t &Size,
                        ArrayRef<uint8_t> B) const {
    if (B.size() < 2) return Fail;
    uint8_t S = B[1];
    auto Unary = [&](unsigned Opcode, unsigned Base) -> DecodeStatus {
      MI.setOpcode(Opcode); addReg(MI, reg(S - Base)); Size = 2; return Success;
    };
    auto PairUnary = [&](unsigned Opcode, unsigned Base) -> DecodeStatus {
      MI.setOpcode(Opcode); addReg(MI, pair(S - Base));
      Size = 2; return Success;
    };
    static const unsigned UnaryOps[] = {AVM::NOT16, AVM::NEG16, AVM::INC16,
      AVM::DEC16, AVM::LSL16, AVM::LSR16, AVM::ASR16, AVM::LSR8,
      AVM::ASR8, 0, 0, AVM::SWAP8, AVM::GETSP, AVM::SETSP,
      AVM::MTPB, AVM::MFPB};
    if (S < 0x20)
      return Unary(UnaryOps[S >> 3], S & 0xf8);
    if (S < 0x24)
      return Unary(AVM::LSL16, 0x20);
    if (S < 0x28) { Size = 2; return Fail; }
    if (S < 0x48)
      return Unary(UnaryOps[S >> 3], S & 0xf8);
    if (S < 0x58) { Size = 2; return Fail; }
    if (S < 0x80)
      return Unary(UnaryOps[S >> 3], S & 0xf8);
    auto Imm16 = [&](unsigned Opcode, unsigned Base) -> DecodeStatus {
      if (B.size() < 4) return Fail; MI.setOpcode(Opcode);
      addReg(MI, reg(S - Base)); addImm(MI, B[2] | uint16_t(B[3]) << 8);
      Size = 4; return Success;
    };
    auto Imm8 = [&](unsigned Opcode, unsigned Base) -> DecodeStatus {
      if (B.size() < 3) return Fail; MI.setOpcode(Opcode);
      addReg(MI, reg(S - Base)); addImm(MI, B[2]); Size = 3; return Success;
    };
    if (S < 0x88) return Imm16(AVM::LDI16, 0x80);
    if (S < 0x8c) return Imm8(AVM::LDI8, 0x88);
    if (S < 0x90) { Size = 2; return Fail; }
    if (S < 0x98) return Imm16(AVM::ADDI16, 0x90);
    if (S < 0xa0) return Imm16(AVM::SUBI16, 0x98);
    if (S < 0xa8) return Imm16(AVM::ANDI16, 0xa0);
    if (S < 0xb0) return Imm16(AVM::ORI16, 0xa8);
    if (S < 0xb8) return Imm16(AVM::XORI16, 0xb0);
    if (S < 0xc0) return Imm16(AVM::CMPI16, 0xb8);
    if (S < 0xc8) return Imm8(AVM::CMPI8, 0xc0);
    if (S < 0xd0) return Unary(AVM::JMPR, 0xc8);
    if (S < 0xd8) return Unary(AVM::CALLR, 0xd0);
    if (S < 0xdc) return PairUnary(AVM::JMPP, 0xd8);
    if (S < 0xe0) { Size = 2; return Fail; }
    if (S < 0xe4) return PairUnary(AVM::CALLP, 0xe0);
    if (S < 0xe8) { Size = 2; return Fail; }
    if (S < 0xec) return Unary(AVM::TST16, 0xe8);
    if (S < 0xf0) { Size = 2; return Fail; }
    if (S < 0xf4) return Unary(AVM::TST8, 0xf0);
    if (S < 0xf8) { Size = 2; return Fail; }
    Size = 2; return Fail;
  }

  DecodeStatus decodeF4(MCInst &MI, uint64_t &Size,
                        ArrayRef<uint8_t> B) const {
    if (B.size() < 2) return Fail;
    uint8_t S = B[1];
    unsigned Op = S >> 4, D = (S >> 2) & 3, Src = S & 3;
    if (Op == 0 || Op == 7 || Op == 8 || Op == 0xf) {
      Size = 2; return Fail;
    }
    if (Op == 2 && D == Src) { Size = 2; return Fail; }
    if (Op >= 3 && Op <= 6 && (D == 0 || D == Src)) {
      Size = 2; return Fail;
    }
    static const unsigned Ops[] = {0, AVM::ADDNF, AVM::SUBNF,
      AVM::AND16, AVM::OR16, AVM::XOR16, AVM::BIC16, AVM::CMP16C,
      AVM::CMP8C, AVM::MULU8, AVM::MULS8, AVM::MULSU8, AVM::SHL16V,
      AVM::LSR16V, AVM::ASR16V};
    MI.setOpcode(Ops[Op]); addReg(MI, compact(D)); addReg(MI, compact(Src));
    Size = 2; return Success;
#if 0
    auto Unary = [&](unsigned Opcode, unsigned Base) -> DecodeStatus {
      MI.setOpcode(Opcode); addReg(MI, reg(S - Base)); Size = 2; return Success;
    };
    if (S <= 0x07) return Unary(AVM::NOT16, 0x00);
    if (S <= 0x0f) return Unary(AVM::NEG16, 0x08);
    if (S <= 0x17) return Unary(AVM::LSL16, 0x10);
    if (S <= 0x1f) return Unary(AVM::LSR16, 0x18);
    if (S <= 0x27) return Unary(AVM::ASR16, 0x20);
    if (S <= 0x2f) return Unary(AVM::LSR8, 0x28);
    if (S <= 0x37) return Unary(AVM::ASR8, 0x30);
    if (S <= 0x3f) return Unary(AVM::ZEXT8, 0x38);
    if (S <= 0x47) return Unary(AVM::SEXT8, 0x40);
    if (S <= 0x4f) return Unary(AVM::SWAP8, 0x48);
    if (S <= 0x57) return Unary(AVM::GETSP, 0x50);
    if (S <= 0x5f) return Unary(AVM::SETSP, 0x58);

    if (S >= 0x60 && S <= 0x6d) {
      if (B.size() < 3) return Fail;
      static const unsigned Ops[] = {AVM::AND16, AVM::OR16, AVM::XOR16,
          AVM::BIC16, AVM::ADC16, AVM::SBC16, AVM::CMP8, AVM::CPC16,
          AVM::MULU8, AVM::MULS8, AVM::MULSU8, AVM::SHL16V,
          AVM::LSR16V, AVM::ASR16V};
      MI.setOpcode(Ops[S - 0x60]);
      if (!decodeRR(MI, B[2])) { Size = 3; return Fail; }
      Size = 3; return Success;
    }
    if (S == 0x6e || S == 0x6f) { Size = 2; return Fail; }

    auto Imm16 = [&](unsigned Opcode, unsigned Base) -> DecodeStatus {
      if (B.size() < 4) return Fail;
      MI.setOpcode(Opcode); addReg(MI, reg(S - Base));
      addImm(MI, B[2] | uint16_t(B[3]) << 8); Size = 4; return Success;
    };
    auto Imm8 = [&](unsigned Opcode, unsigned Base) -> DecodeStatus {
      if (B.size() < 3) return Fail;
      MI.setOpcode(Opcode); addReg(MI, reg(S - Base));
      addImm(MI, B[2]); Size = 3; return Success;
    };
    if (S >= 0x70 && S <= 0x77) return Imm16(AVM::LDI16, 0x70);
    if (S >= 0x78 && S <= 0x7f) return Imm8(AVM::LDI8, 0x78);
    if (S >= 0x80 && S <= 0x87) return Imm16(AVM::ADDI16, 0x80);
    if (S >= 0x88 && S <= 0x8f) return Imm16(AVM::SUBI16, 0x88);
    if (S >= 0x90 && S <= 0x97) return Imm16(AVM::ANDI16, 0x90);
    if (S >= 0x98 && S <= 0x9f) return Imm16(AVM::ORI16, 0x98);
    if (S >= 0xa0 && S <= 0xa7) return Imm16(AVM::XORI16, 0xa0);
    if (S >= 0xa8 && S <= 0xaf) return Imm16(AVM::CMPI16, 0xa8);
    if (S >= 0xb0 && S <= 0xb7) return Imm8(AVM::CMPI8, 0xb0);

    if (S >= 0xb8 && S <= 0xbb) {
      if (B.size() < 3) return Fail;
      static const unsigned Ops[] = {AVM::MOV16, AVM::ADD16,
                                     AVM::SUB16, AVM::CMP16};
      MI.setOpcode(Ops[S - 0xb8]);
      if (!decodeRR(MI, B[2])) { Size = 3; return Fail; }
      Size = 3; return Success;
    }
    if (S == 0xbc) {
      if (B.size() < 3) return Fail;
      uint8_t X = B[2];
      MI.setOpcode(AVM::CMPI6); addReg(MI, compact(X & 3));
      int8_t Imm = static_cast<int8_t>(X) >> 2;
      addImm(MI, Imm); Size = 3; return Success;
    }
    if (S == 0xbd || S == 0xbe || S == 0xbf) {
      if (B.size() < 3) return Fail;
      MI.setOpcode(S == 0xbd ? AVM::JMP_REL8
                   : S == 0xbe ? AVM::CALL_REL8 : AVM::ADJSP);
      addImm(MI, static_cast<int8_t>(B[2])); Size = 3; return Success;
    }
    if (S >= 0xc0 && S <= 0xef) {
      unsigned Op;
      unsigned Base;
      if (S <= 0xc7) { Op = AVM::JMPR; Base = 0xc0; }
      else if (S <= 0xcf) { Op = AVM::CALLR; Base = 0xc8; }
      else if (S <= 0xd7) { Op = AVM::JMPP; Base = 0xd0; }
      else if (S <= 0xdf) { Op = AVM::CALLP; Base = 0xd8; }
      else if (S <= 0xe7) { Op = AVM::MTPB; Base = 0xe0; }
      else { Op = AVM::MFPB; Base = 0xe8; }
      return Unary(Op, Base);
    }
    if (S == 0xf0) {
      if (B.size() < 3) return Fail;
      MI.setOpcode(AVM::LDPBI); addImm(MI, B[2]); Size = 3; return Success;
    }
    if (S == 0xf1 || S == 0xf2) {
      if (B.size() < 4) return Fail;
      MI.setOpcode(S == 0xf1 ? AVM::JMP16 : AVM::CALL16);
      addImm(MI, B[2] | uint16_t(B[3]) << 8); Size = 4; return Success;
    }
    if (S == 0xf3) { MI.setOpcode(AVM::NOP); Size = 2; return Success; }
    if (S == 0xf4) {
      if (B.size() < 3) return Fail;
      MI.setOpcode(AVM::SYS); addImm(MI, B[2]); Size = 3; return Success;
    }
    if (S == 0xf9 || S == 0xfa) {
      if (B.size() < 3) return Fail;
      uint8_t X = B[2]; bool Word = X & 0x80;
      bool Store = S == 0xfa;
      MI.setOpcode(Store ? (Word ? AVM::STSP16C : AVM::STSP8C)
                         : (Word ? AVM::LDSP16C : AVM::LDSP8C));
      if (Store) { addImm(MI, X & 0x1f); addReg(MI, compact((X >> 5) & 3)); }
      else { addReg(MI, compact((X >> 5) & 3)); addImm(MI, X & 0x1f); }
      Size = 3; return Success;
    }
    Size = 2; return Fail;
#endif
  }

  DecodeStatus decodeFD(MCInst &MI, uint64_t &Size,
                        ArrayRef<uint8_t> B) const {
    if (B.size() < 2) return Fail;
    uint8_t S = B[1];
    if (S <= 0x0c) {
      if (B.size() < 3) return Fail;
      static const unsigned Ops[] = {AVM::LD8, AVM::ST8, AVM::LD16,
          AVM::ST16, AVM::LD8_POST, AVM::ST8_POST, AVM::LD16_POST,
          AVM::ST16_POST, AVM::LEA, AVM::LD8_DISP, AVM::ST8_DISP,
          AVM::LD16_DISP, AVM::ST16_DISP};
      MI.setOpcode(Ops[S]);
      uint8_t RR = B[2];
      if (RR & 0x11) { Size = 3; return Fail; }
      MCRegister D = reg((RR >> 5) & 7), A = reg((RR >> 1) & 7);
      bool Store = S == 1 || S == 3 || S == 5 || S == 7 || S == 10 || S == 12;
      bool Disp = S >= 8;
      if (Disp && B.size() < 4) return Fail;
      if (Store) {
        addReg(MI, D);
        if (Disp) addImm(MI, static_cast<int8_t>(B[3]));
        addReg(MI, A);
      } else {
        addReg(MI, D); addReg(MI, A);
        if (Disp) addImm(MI, static_cast<int8_t>(B[3]));
      }
      Size = Disp ? 4 : 3;
      return Success;
    }
    if (S >= 0x10 && S <= 0x2f) {
      if (B.size() < 3) return Fail;
      bool Store = (S >= 0x18 && S <= 0x1f) || S >= 0x28;
      bool Word = S >= 0x20;
      MI.setOpcode(Store ? (Word ? AVM::STSP16 : AVM::STSP8)
                         : (Word ? AVM::LDSP16 : AVM::LDSP8));
      if (Store) { addImm(MI, B[2]); addReg(MI, reg(S & 7)); }
      else { addReg(MI, reg(S & 7)); addImm(MI, B[2]); }
      Size = 3; return Success;
    }
    if (S >= 0x30 && S <= 0x4f) {
      if (B.size() < 4) return Fail;
      bool Store = (S >= 0x38 && S <= 0x3f) || S >= 0x48;
      bool Word = S >= 0x40;
      MI.setOpcode(Store ? (Word ? AVM::STM16 : AVM::STM8)
                         : (Word ? AVM::LDM16 : AVM::LDM8));
      uint16_t Addr = B[2] | uint16_t(B[3]) << 8;
      if (Store) { addImm(MI, Addr); addReg(MI, reg(S & 7)); }
      else { addReg(MI, reg(S & 7)); addImm(MI, Addr); }
      Size = 4; return Success;
    }
    if (S >= 0x80 && S <= 0x83) {
      if (B.size() < 3) return Fail;
      MI.setOpcode(S == 0x80 ? AVM::LDP8 : S == 0x81 ? AVM::LDP16
                   : S == 0x82 ? AVM::LDP8_DISP : AVM::LDP16_DISP);
      if (!decodeRR(MI, B[2])) { Size = 3; return Fail; }
      if (S >= 0x82) {
        if (B.size() < 4) return Fail;
        addImm(MI, static_cast<int8_t>(B[3])); Size = 4;
      } else Size = 3;
      return Success;
    }
    Size = 2;
    return Fail;
  }
};

} // namespace

static MCDisassembler *createAVMDisassembler(const Target &,
                                              const MCSubtargetInfo &STI,
                                              MCContext &Ctx) {
  return new AVMDisassembler(STI, Ctx);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVMDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheAVMTarget(),
                                          createAVMDisassembler);
}
