//===-- ABCInstrFormats.h - ABC bytecode instruction table ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCINSTRFORMATS_H
#define LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCINSTRFORMATS_H

#include "ABCMCTargetDesc.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include <cstdint>

namespace llvm {
namespace ABC {

enum class ImmWidth : uint8_t {
  None = 0,
  U8 = 1,
  U16 = 2,
  U24 = 3,
  U32 = 4,
};

enum class Fixup : uint8_t {
  None,
  Abs8,
  Abs16,
  Abs24,
  Abs32,
  Prog24,
  Global16Tagged,
  Global8,
  Branch8,
  Branch16,
  Call24,
};

struct InstrDesc {
  unsigned Opcode;
  const char *Mnemonic;
  uint8_t Bytecode;
  uint8_t NumOperands;
  ImmWidth Widths[3];
  Fixup Fixups[3];
};

inline ArrayRef<InstrDesc> getInstrTable() {
#define ABC0(NAME, MNEMONIC, BYTE)                                             \
  {ABC::NAME, MNEMONIC, BYTE, 0,                                               \
   {ImmWidth::None, ImmWidth::None, ImmWidth::None},                           \
   {Fixup::None, Fixup::None, Fixup::None}}
#define ABC1(NAME, MNEMONIC, BYTE, W0, F0)                                     \
  {ABC::NAME, MNEMONIC, BYTE, 1,                                               \
   {ImmWidth::W0, ImmWidth::None, ImmWidth::None},                             \
   {Fixup::F0, Fixup::None, Fixup::None}}
#define ABC2(NAME, MNEMONIC, BYTE, W0, F0, W1, F1)                             \
  {ABC::NAME, MNEMONIC, BYTE, 2,                                               \
   {ImmWidth::W0, ImmWidth::W1, ImmWidth::None},                               \
   {Fixup::F0, Fixup::F1, Fixup::None}}
  static const InstrDesc Table[] = {
      ABC0(NOP, "nop", 0x00),
      ABC1(PUSH, "push", 0x01, U8, Abs8),
      ABC0(P0, "p0", 0x02),
      ABC0(P1, "p1", 0x03),
      ABC0(P2, "p2", 0x04),
      ABC0(P3, "p3", 0x05),
      ABC0(P4, "p4", 0x06),
      ABC0(P5, "p5", 0x07),
      ABC0(P6, "p6", 0x08),
      ABC0(P7, "p7", 0x09),
      ABC0(P8, "p8", 0x0a),
      ABC0(P16, "p16", 0x0b),
      ABC0(P32, "p32", 0x0c),
      ABC0(P64, "p64", 0x0d),
      ABC0(P128, "p128", 0x0e),
      ABC0(P00, "p00", 0x0f),
      ABC0(P000, "p000", 0x10),
      ABC0(P0000, "p0000", 0x11),
      ABC0(PZ8, "pz8", 0x12),
      ABC0(PZ16, "pz16", 0x13),
      // A symbolic pushg operand is a global address, not a plain immediate.
      // Use the global relocation so the encoded address includes the global
      // RAM base (0x0200), just like getg*/setg* operands do.
      ABC1(PUSHG, "pushg", 0x14, U16, Global16Tagged),
      ABC1(PUSH2, "push2", 0x14, U16, Abs16),
      ABC1(PUSHL, "pushl", 0x15, U24, Prog24),
      ABC1(PUSH3, "push3", 0x15, U24, Abs24),
      ABC1(PUSH4, "push4", 0x16, U32, Abs32),
      ABC0(SEXT, "sext", 0x17),
      ABC0(SEXT2, "sext2", 0x18),
      ABC0(SEXT3, "sext3", 0x19),
      ABC0(DUP, "dup", 0x1a),
      ABC0(DUP2, "dup2", 0x1b),
      ABC0(DUP3, "dup3", 0x1c),
      ABC0(DUP4, "dup4", 0x1d),
      ABC0(DUP5, "dup5", 0x1e),
      ABC0(DUP6, "dup6", 0x1f),
      ABC0(DUP7, "dup7", 0x20),
      ABC0(DUP8, "dup8", 0x21),
      ABC0(DUPW, "dupw", 0x22),
      ABC0(DUPW2, "dupw2", 0x23),
      ABC0(DUPW3, "dupw3", 0x24),
      ABC0(DUPW4, "dupw4", 0x25),
      ABC0(DUPW5, "dupw5", 0x26),
      ABC0(DUPW6, "dupw6", 0x27),
      ABC0(DUPW7, "dupw7", 0x28),
      ABC0(DUPW8, "dupw8", 0x29),
      ABC1(GETL, "getl", 0x2a, U8, Abs8),
      ABC1(GETL2, "getl2", 0x2b, U8, Abs8),
      ABC1(GETL4, "getl4", 0x2c, U8, Abs8),
      ABC2(GETLN, "getln", 0x2d, U8, Abs8, U8, Abs8),
      ABC1(SETL, "setl", 0x2e, U8, Abs8),
      ABC1(SETL2, "setl2", 0x2f, U8, Abs8),
      ABC1(SETL4, "setl4", 0x30, U8, Abs8),
      ABC2(SETLN, "setln", 0x31, U8, Abs8, U8, Abs8),
      ABC1(GETG, "getg", 0x32, U16, Global16Tagged),
      ABC1(GETG2, "getg2", 0x33, U16, Global16Tagged),
      ABC1(GETG4, "getg4", 0x34, U16, Global16Tagged),
      ABC2(GETGN, "getgn", 0x35, U8, Abs8, U16, Global16Tagged),
      ABC1(GTGB, "gtgb", 0x36, U8, Global8),
      ABC1(GTGB2, "gtgb2", 0x37, U8, Global8),
      ABC1(GTGB4, "gtgb4", 0x38, U8, Global8),
      ABC1(SETG, "setg", 0x39, U16, Global16Tagged),
      ABC1(SETG2, "setg2", 0x3a, U16, Global16Tagged),
      ABC1(SETG4, "setg4", 0x3b, U16, Global16Tagged),
      ABC2(SETGN, "setgn", 0x3c, U8, Abs8, U16, Global16Tagged),
      ABC0(GETP, "getp", 0x3d),
      ABC1(GETPN, "getpn", 0x3e, U8, Abs8),
      ABC0(GETR, "getr", 0x3f),
      ABC0(GETR2, "getr2", 0x40),
      ABC1(GETRN, "getrn", 0x41, U8, Abs8),
      ABC0(SETR, "setr", 0x42),
      ABC0(SETR2, "setr2", 0x43),
      ABC1(SETRN, "setrn", 0x44, U8, Abs8),
      ABC0(POP, "pop", 0x45),
      ABC0(POP2, "pop2", 0x46),
      ABC0(POP3, "pop3", 0x47),
      ABC0(POP4, "pop4", 0x48),
      ABC1(POPN, "popn", 0x49, U8, Abs8),
      ABC1(ALLOC, "alloc", 0x4a, U8, Abs8),
      ABC1(AIXB1, "aixb1", 0x4b, U8, Abs8),
      ABC2(AIDXB, "aidxb", 0x4c, U8, Abs8, U8, Abs8),
      ABC2(AIDX, "aidx", 0x4d, U16, Abs16, U16, Abs16),
      ABC2(PIDXB, "pidxb", 0x4e, U8, Abs8, U8, Abs8),
      ABC2(PIDX, "pidx", 0x4f, U16, Abs16, U24, Abs24),
      ABC1(UAIDX, "uaidx", 0x50, U16, Abs16),
      ABC1(UPIDX, "upidx", 0x51, U16, Abs16),
      ABC1(REFL, "refl", 0x54, U8, Abs8),
      ABC1(REFGB, "refgb", 0x55, U8, Global8),
      ABC0(INC, "inc", 0x56),
      ABC0(DEC, "dec", 0x57),
      ABC1(LINC, "linc", 0x58, U8, Abs8),
      ABC0(PINC, "pinc", 0x59),
      ABC0(PINC2, "pinc2", 0x5a),
      ABC0(PINC3, "pinc3", 0x5b),
      ABC0(PINC4, "pinc4", 0x5c),
      ABC0(PDEC, "pdec", 0x5d),
      ABC0(PDEC2, "pdec2", 0x5e),
      ABC0(PDEC3, "pdec3", 0x5f),
      ABC0(PDEC4, "pdec4", 0x60),
      ABC0(PINCF, "pincf", 0x61),
      ABC0(PDECF, "pdecf", 0x62),
      ABC0(ADD, "add", 0x63),
      ABC0(ADD2, "add2", 0x64),
      ABC0(ADD3, "add3", 0x65),
      ABC0(ADD4, "add4", 0x66),
      ABC0(SUB, "sub", 0x67),
      ABC0(SUB2, "sub2", 0x68),
      ABC0(SUB3, "sub3", 0x69),
      ABC0(SUB4, "sub4", 0x6a),
      ABC0(ADD2B, "add2b", 0x6b),
      ABC0(ADD3B, "add3b", 0x6c),
      ABC0(SUB2B, "sub2b", 0x6d),
      ABC0(MUL2B, "mul2b", 0x6e),
      ABC0(MUL, "mul", 0x6f),
      ABC0(MUL2, "mul2", 0x70),
      ABC0(MUL3, "mul3", 0x71),
      ABC0(MUL4, "mul4", 0x72),
      ABC0(UDIV2, "udiv2", 0x73),
      ABC0(UDIV4, "udiv4", 0x74),
      ABC0(DIV2, "div2", 0x75),
      ABC0(DIV4, "div4", 0x76),
      ABC0(UMOD2, "umod2", 0x77),
      ABC0(UMOD4, "umod4", 0x78),
      ABC0(MOD2, "mod2", 0x79),
      ABC0(MOD4, "mod4", 0x7a),
      ABC0(LSL, "lsl", 0x7b),
      ABC0(LSL2, "lsl2", 0x7c),
      ABC0(LSL4, "lsl4", 0x7d),
      ABC0(LSR, "lsr", 0x7e),
      ABC0(LSR2, "lsr2", 0x7f),
      ABC0(LSR4, "lsr4", 0x80),
      ABC0(ASR, "asr", 0x81),
      ABC0(ASR2, "asr2", 0x82),
      ABC0(ASR4, "asr4", 0x83),
      ABC0(AND, "and", 0x84),
      ABC0(AND2, "and2", 0x85),
      ABC0(AND4, "and4", 0x86),
      ABC0(OR, "or", 0x87),
      ABC0(OR2, "or2", 0x88),
      ABC0(OR4, "or4", 0x89),
      ABC0(XOR, "xor", 0x8a),
      ABC0(XOR2, "xor2", 0x8b),
      ABC0(XOR4, "xor4", 0x8c),
      ABC0(COMP, "comp", 0x8d),
      ABC0(COMP2, "comp2", 0x8e),
      ABC0(COMP4, "comp4", 0x8f),
      ABC0(BOOL, "bool", 0x90),
      ABC0(BOOL2, "bool2", 0x91),
      ABC0(BOOL3, "bool3", 0x92),
      ABC0(BOOL4, "bool4", 0x93),
      ABC0(CULT, "cult", 0x94),
      ABC0(CULT2, "cult2", 0x95),
      ABC0(CULT3, "cult3", 0x96),
      ABC0(CULT4, "cult4", 0x97),
      ABC0(CSLT, "cslt", 0x98),
      ABC0(CSLT2, "cslt2", 0x99),
      ABC0(CSLT3, "cslt3", 0x9a),
      ABC0(CSLT4, "cslt4", 0x9b),
      ABC0(CFEQ, "cfeq", 0x9c),
      ABC0(CFLT, "cflt", 0x9d),
      ABC0(NOT, "not", 0x9e),
      ABC0(FADD, "fadd", 0x9f),
      ABC0(FSUB, "fsub", 0xa0),
      ABC0(FMUL, "fmul", 0xa1),
      ABC0(FDIV, "fdiv", 0xa2),
      ABC0(F2I, "f2i", 0xa3),
      ABC0(F2U, "f2u", 0xa4),
      ABC0(I2F, "i2f", 0xa5),
      ABC0(U2F, "u2f", 0xa6),
      ABC1(BZ, "bz", 0xa7, U24, Prog24),
      ABC1(BZ1, "bz1", 0xa8, U8, Branch8),
      ABC1(BZ2, "bz2", 0xa9, U16, Branch16),
      ABC1(BNZ, "bnz", 0xaa, U24, Prog24),
      ABC1(BNZ1, "bnz1", 0xab, U8, Branch8),
      ABC1(BNZ2, "bnz2", 0xac, U16, Branch16),
      ABC1(BZP, "bzp", 0xad, U24, Prog24),
      ABC1(BZP1, "bzp1", 0xae, U8, Branch8),
      ABC1(BNZP, "bnzp", 0xaf, U24, Prog24),
      ABC1(BNZP1, "bnzp1", 0xb0, U8, Branch8),
      ABC1(JMP, "jmp", 0xb1, U24, Prog24),
      ABC1(JMP1, "jmp1", 0xb2, U8, Branch8),
      ABC1(JMP2, "jmp2", 0xb3, U16, Branch16),
      ABC0(IJMP, "ijmp", 0xb4),
      ABC1(CALL, "call", 0xb5, U24, Call24),
      ABC1(CALL1, "call1", 0xb6, U8, Branch8),
      ABC1(CALL2, "call2", 0xb7, U16, Branch16),
      ABC0(ICALL, "icall", 0xb8),
      ABC0(RET, "ret", 0xb9),
      ABC1(SYS, "sys", 0xba, U8, Abs8),
  };
  return Table;
#undef ABC2
#undef ABC1
#undef ABC0
}

inline const InstrDesc *getInstrDescByOpcode(unsigned Opcode) {
  for (const InstrDesc &Desc : getInstrTable())
    if (Desc.Opcode == Opcode)
      return &Desc;
  return nullptr;
}

inline const InstrDesc *getInstrDescByBytecode(uint8_t Bytecode) {
  for (const InstrDesc &Desc : getInstrTable())
    if (Desc.Bytecode == Bytecode)
      return &Desc;
  return nullptr;
}

inline const InstrDesc *getInstrDescByMnemonic(StringRef Mnemonic) {
  for (const InstrDesc &Desc : getInstrTable())
    if (Mnemonic.equals_insensitive(Desc.Mnemonic))
      return &Desc;
  return nullptr;
}

} // namespace ABC
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCINSTRFORMATS_H
