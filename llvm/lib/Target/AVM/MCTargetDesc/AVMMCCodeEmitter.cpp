#include "AVMFixupKinds.h"
#include "AVMMCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/MathExtras.h"
#include <optional>

using namespace llvm;

namespace {

class AVMMCCodeEmitter final : public MCCodeEmitter {
  MCContext &Ctx;

  void error(const MCInst &MI, const Twine &Message) const {
    Ctx.reportError(MI.getLoc(), Message);
  }

  static void emit8(SmallVectorImpl<char> &Out, uint64_t Value) {
    Out.push_back(static_cast<char>(Value));
  }

  static void emit24(SmallVectorImpl<char> &Out, uint64_t Value) {
    emit8(Out, Value);
    emit8(Out, Value >> 8);
    emit8(Out, Value >> 16);
  }

  static void emit16(SmallVectorImpl<char> &Out, uint64_t Value) {
    emit8(Out, Value);
    emit8(Out, Value >> 8);
  }

  void emitFarTarget(const MCInst &MI, SmallVectorImpl<char> &Out,
                     SmallVectorImpl<MCFixup> &Fixups) const {
    const MCOperand &Operand = MI.getOperand(0);
    if (Operand.isImm()) {
      if (!isUInt<24>(Operand.getImm()))
        error(MI, "far target is out of 24-bit range");
      emit24(Out, Operand.getImm());
      return;
    }
    if (!Operand.isExpr()) {
      error(MI, "expected far target expression");
      emit24(Out, 0);
      return;
    }
    Fixups.push_back(
        MCFixup::create(1, Operand.getExpr(), AVM::fixup_avm_far24));
    emit24(Out, 0);
  }

  void emitRel8(const MCInst &MI, SmallVectorImpl<char> &Out,
                SmallVectorImpl<MCFixup> &Fixups, unsigned Opcode) const {
    if (MI.getNumOperands() != 1) {
      error(MI, "expected one relative displacement operand");
      return;
    }
    emit8(Out, Opcode);
    const MCOperand &Operand = MI.getOperand(0);
    if (Operand.isImm()) {
      if (!isInt<8>(Operand.getImm()))
        error(MI, "relative displacement is out of signed 8-bit range");
      emit8(Out, Operand.getImm());
      return;
    }
    if (!Operand.isExpr()) {
      error(MI, "expected relative displacement expression");
      emit8(Out, 0);
      return;
    }
    Fixups.push_back(
        MCFixup::create(1, Operand.getExpr(), AVM::fixup_avm_pcrel8, true));
    emit8(Out, 0);
  }

  void emitRel16(const MCInst &MI, SmallVectorImpl<char> &Out,
                 SmallVectorImpl<MCFixup> &Fixups, unsigned Opcode) const {
    if (MI.getNumOperands() != 1) {
      error(MI, "expected one relative displacement operand");
      return;
    }
    emit8(Out, Opcode);
    const MCOperand &Operand = MI.getOperand(0);
    if (Operand.isImm()) {
      if (!isInt<16>(Operand.getImm()))
        error(MI, "relative displacement is out of signed 16-bit range");
      emit8(Out, Operand.getImm());
      emit8(Out, Operand.getImm() >> 8);
      return;
    }
    if (!Operand.isExpr()) {
      error(MI, "expected relative displacement expression");
      emit8(Out, 0);
      emit8(Out, 0);
      return;
    }
    Fixups.push_back(
        MCFixup::create(1, Operand.getExpr(), AVM::fixup_avm_pcrel16, true));
    emit8(Out, 0);
    emit8(Out, 0);
  }

  void emitSigned8(const MCInst &MI, SmallVectorImpl<char> &Out,
                   unsigned Opcode) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isImm()) {
      error(MI, "expected one signed 8-bit immediate operand");
      return;
    }
    const int64_t Value = MI.getOperand(0).getImm();
    if (!isInt<8>(Value)) {
      error(MI, "immediate is out of signed 8-bit range");
      return;
    }
    emit8(Out, Opcode);
    emit8(Out, Value);
  }

  void emitService(const MCInst &MI, SmallVectorImpl<char> &Out) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isImm() ||
        MI.getOperand(0).getImm() < 0 || MI.getOperand(0).getImm() > 3) {
      error(MI, "invalid AVM version 1 service identifier");
      return;
    }
    emit8(Out, 0xd7);
    emit8(Out, MI.getOperand(0).getImm());
  }

  static std::optional<unsigned> compactRegIndex(MCRegister Reg) {
    switch (Reg.id()) {
    case AVM::R4: return 0;
    case AVM::R5: return 1;
    case AVM::R6: return 2;
    case AVM::R7: return 3;
    default: return std::nullopt;
    }
  }

  static std::optional<unsigned> stackRegIndex(MCRegister Reg) {
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

  static std::optional<unsigned> pair48Index(MCRegister Left,
                                              MCRegister Right) {
    const auto L = stackRegIndex(Left);
    const auto R = stackRegIndex(Right);
    if (!L || !R || (*L >= 4 && *R >= 4))
      return std::nullopt;
    return *L < 4 ? 8 * *L + *R : 0x20 + 4 * (*L - 4) + *R;
  }

  void emitFullMove(const MCInst &MI, SmallVectorImpl<char> &Out) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected two full register operands");
      return;
    }
    const auto Pair = pair48Index(MI.getOperand(0).getReg(),
                                  MI.getOperand(1).getReg());
    if (!Pair) {
      error(MI, "full-register MOV pairing is not encodable");
      return;
    }
    emit8(Out, 0xf1);
    emit8(Out, *Pair);
  }

  void emitFullArithmetic(const MCInst &MI, SmallVectorImpl<char> &Out,
                          unsigned SecondaryBase) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected two full register operands");
      return;
    }
    const auto Pair = pair48Index(MI.getOperand(0).getReg(),
                                  MI.getOperand(1).getReg());
    if (!Pair) {
      error(MI, "full-register arithmetic pairing is not encodable; use compact cN spelling");
      return;
    }
    emit8(Out, 0xf2);
    emit8(Out, SecondaryBase + *Pair);
  }

  void emitFullBitwise(const MCInst &MI, SmallVectorImpl<char> &Out,
                       unsigned Operation) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected two full register operands");
      return;
    }
    const auto D = stackRegIndex(MI.getOperand(0).getReg());
    const auto S = stackRegIndex(MI.getOperand(1).getReg());
    if (!D || !S) {
      error(MI, "expected full registers r0-r7");
      return;
    }
    emit8(Out, 0xf9);
    emit8(Out, (*D << 5) | (*S << 2) | Operation);
  }

  void emitFullCompare(const MCInst &MI, SmallVectorImpl<char> &Out) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected two full register operands");
      return;
    }
    const auto Pair = pair48Index(MI.getOperand(0).getReg(),
                                  MI.getOperand(1).getReg());
    if (!Pair) {
      error(MI, "cmp full-register pairing is not encodable; use compact cN spelling");
      return;
    }
    emit8(Out, 0xf5);
    emit8(Out, *Pair);
  }

  // This mapping is architectural, not derived from physical-register enum
  // layout.  The dddWaaaP encoding uses these r0-r7 values directly.
  static std::optional<unsigned> generalPointerRegIndex(MCRegister Reg) {
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

  static std::optional<unsigned> coldRegIndex(MCRegister Reg) {
    switch (Reg.id()) {
    case AVM::R0: return 0;
    case AVM::R1: return 1;
    case AVM::R2: return 2;
    case AVM::R3: return 3;
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

  void emitCold32(const MCInst &MI, SmallVectorImpl<char> &Out,
                  unsigned Secondary, bool IsCompare, bool IsStore) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected two register operands");
      return;
    }
    const MCRegister HighReg = MI.getOperand(IsStore ? 1 : 0).getReg();
    const MCRegister LowReg = MI.getOperand(IsStore ? 0 : 1).getReg();
    const auto High = programPairIndex(HighReg);
    const auto Low = IsCompare ? programPairIndex(LowReg)
                               : stackRegIndex(LowReg);
    if (!High || !Low) {
      error(MI, IsCompare ? "expected pair operands q0-q3"
                          : "expected pair and address operands");
      return;
    }
    emit8(Out, 0xf0);
    emit8(Out, Secondary);
    emit8(Out, (*High * 4) << 4 | (*Low * (IsCompare ? 4 : 2)));
  }

  void emitProgramLoad(const MCInst &MI, SmallVectorImpl<char> &Out,
                       unsigned Secondary, bool IsPair) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected program-load destination and address pair operands");
      return;
    }
    const std::optional<unsigned> Address =
        programPairIndex(MI.getOperand(1).getReg());
    const std::optional<unsigned> Destination =
        IsPair ? programPairIndex(MI.getOperand(0).getReg())
               : stackRegIndex(MI.getOperand(0).getReg());
    if (!Destination || !Address) {
      error(MI, "invalid program-load register operands");
      return;
    }
    emit8(Out, 0xf0);
    emit8(Out, Secondary);
    emit8(Out, (IsPair ? 4 * *Destination : 2 * *Destination) << 4 |
                   4 * *Address);
  }

  void emitGeneralPointer(const MCInst &MI, SmallVectorImpl<char> &Out,
                          unsigned Secondary, bool IsWord, bool IsPost,
                          bool IsStore) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected general-pointer register memory operands");
      return;
    }
    const MCRegister Data = MI.getOperand(IsStore ? 1 : 0).getReg();
    const MCRegister Address = MI.getOperand(IsStore ? 0 : 1).getReg();
    const auto DataIndex = generalPointerRegIndex(Data);
    const auto AddressIndex = generalPointerRegIndex(Address);
    if (!DataIndex || !AddressIndex) {
      error(MI, "expected full register r0-r7 operands");
      return;
    }
    if (!IsStore && IsPost && *DataIndex == *AddressIndex) {
      error(MI, "postincrement destination must not overlap address register");
      return;
    }
    emit8(Out, 0xf0);
    emit8(Out, Secondary);
    emit8(Out, (*DataIndex << 5) | (unsigned(IsWord) << 4) |
                   (*AddressIndex << 1) | unsigned(IsPost));
  }

  void emitF7Memory(const MCInst &MI, SmallVectorImpl<char> &Out,
                    unsigned Base, bool IsStore) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected compact-pointer postincrement register operands");
      return;
    }
    const MCRegister Data = MI.getOperand(IsStore ? 1 : 0).getReg();
    const MCRegister Address = MI.getOperand(IsStore ? 0 : 1).getReg();
    const auto DataIndex = stackRegIndex(Data);
    const auto AddressIndex = compactRegIndex(Address);
    if (!DataIndex || !AddressIndex) {
      error(MI, IsStore ? "expected compact pointer c0-c3 and source r0-r7"
                        : "expected compact pointer c0-c3 and destination r0-r7");
      return;
    }
    if (!IsStore && *DataIndex == 4 + *AddressIndex) {
      error(MI, "postincrement destination must not overlap address register");
      return;
    }
    emit8(Out, 0xf7);
    emit8(Out, Base + 8 * *AddressIndex + *DataIndex);
  }

  void emitProgramPair(const MCInst &MI, SmallVectorImpl<char> &Out,
                       unsigned Family) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isReg()) {
      error(MI, "expected one program pair q0-q3");
      return;
    }
    const std::optional<unsigned> Index =
        programPairIndex(MI.getOperand(0).getReg());
    if (!Index) {
      error(MI, "expected program pair q0-q3");
      return;
    }
    emit8(Out, Family | *Index);
  }

  void emitF7PairArithmetic(const MCInst &MI, SmallVectorImpl<char> &Out,
                            unsigned Base) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected two pair operands q0-q3");
      return;
    }
    const auto D = programPairIndex(MI.getOperand(0).getReg());
    const auto S = programPairIndex(MI.getOperand(1).getReg());
    if (!D || !S) {
      error(MI, "expected pair operands q0-q3");
      return;
    }
    emit8(Out, 0xf7);
    emit8(Out, Base + 4 * *D + *S);
  }

  void emitF7PairUnary(const MCInst &MI, SmallVectorImpl<char> &Out,
                       unsigned Base) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isReg()) {
      error(MI, "expected one pair operand q0-q3");
      return;
    }
    const auto D = programPairIndex(MI.getOperand(0).getReg());
    if (!D) {
      error(MI, "expected pair operand q0-q3");
      return;
    }
    emit8(Out, 0xf7);
    emit8(Out, Base + *D);
  }

  void emitF7Bool(const MCInst &MI, SmallVectorImpl<char> &Out) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isReg()) {
      error(MI, "expected one full register operand r0-r7");
      return;
    }
    const auto D = generalPointerRegIndex(MI.getOperand(0).getReg());
    if (!D) {
      error(MI, "expected full register operand r0-r7");
      return;
    }
    emit8(Out, 0xf7);
    emit8(Out, 0x88 + *D);
  }

  void emitStackReg(const MCInst &MI, SmallVectorImpl<char> &Out,
                    unsigned Family) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isReg()) {
      error(MI, "expected one full register operand");
      return;
    }
    const std::optional<unsigned> Index =
        stackRegIndex(MI.getOperand(0).getReg());
    if (!Index) {
      error(MI, "expected full register r0-r7");
      return;
    }
    emit8(Out, Family | *Index);
  }

  void emitCompactMatrix(const MCInst &MI, SmallVectorImpl<char> &Out,
                         unsigned Family) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected two compact register operands");
      return;
    }
    const std::optional<unsigned> First =
        compactRegIndex(MI.getOperand(0).getReg());
    const std::optional<unsigned> Second =
        compactRegIndex(MI.getOperand(1).getReg());
    if (!First || !Second) {
      error(MI, "expected compact register c0-c3");
      return;
    }
    emit8(Out, Family | (*First << 2) | *Second);
  }

  void emitF3Store(const MCInst &MI, SmallVectorImpl<char> &Out) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected compact pointer and r0-r3 source operands");
      return;
    }
    const auto Address = compactRegIndex(MI.getOperand(0).getReg());
    const auto Source = stackRegIndex(MI.getOperand(1).getReg());
    if (!Address || !Source || *Source > 3) {
      error(MI, "expected compact pointer c0-c3 and source r0-r3");
      return;
    }
    emit8(Out, 0xf3);
    emit8(Out, 4 * *Address + *Source);
  }

  void emitF6PostStore(const MCInst &MI,
                       SmallVectorImpl<char> &Out) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected compact pointer and r0-r7 source operands");
      return;
    }
    const auto Address = compactRegIndex(MI.getOperand(0).getReg());
    const auto Source = stackRegIndex(MI.getOperand(1).getReg());
    if (!Address || !Source) {
      error(MI, "expected compact pointer c0-c3 and source r0-r7");
      return;
    }
    emit8(Out, 0xf6);
    emit8(Out, 8 * *Address + *Source);
  }

  void emitF5Memory(const MCInst &MI, SmallVectorImpl<char> &Out,
                    unsigned Base, bool IsStore) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, IsStore ? "expected compact pointer and r0-r3 source operands"
                        : "expected r0-r3 destination and compact pointer operands");
      return;
    }
    const MCRegister AddressReg =
        IsStore ? MI.getOperand(0).getReg() : MI.getOperand(1).getReg();
    const MCRegister DataReg =
        IsStore ? MI.getOperand(1).getReg() : MI.getOperand(0).getReg();
    const auto Address = compactRegIndex(AddressReg);
    const auto Data = stackRegIndex(DataReg);
    if (!Address || !Data || *Data > 3) {
      error(MI, IsStore ? "expected compact pointer c0-c3 and source r0-r3"
                        : "expected destination register r0-r3 and compact pointer c0-c3");
      return;
    }
    emit8(Out, 0xf5);
    emit8(Out, Base + 4 * *Address + *Data);
  }

  void emitF3Multiply(const MCInst &MI, SmallVectorImpl<char> &Out,
                      unsigned Family) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected two compact register operands");
      return;
    }
    const auto Destination = compactRegIndex(MI.getOperand(0).getReg());
    const auto Source = compactRegIndex(MI.getOperand(1).getReg());
    if (!Destination || !Source) {
      error(MI, "expected compact register c0-c3");
      return;
    }
    emit8(Out, 0xf3);
    emit8(Out, Family + 4 * *Destination + *Source);
  }

  void emitCompactImmediate(const MCInst &MI, SmallVectorImpl<char> &Out,
                            unsigned Family, unsigned Bits, bool IsSigned) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isImm()) {
      error(MI, "expected compact register and immediate operands");
      return;
    }
    const std::optional<unsigned> Reg = compactRegIndex(MI.getOperand(0).getReg());
    if (!Reg) {
      error(MI, "expected compact register c0-c3");
      return;
    }
    const int64_t Value = MI.getOperand(1).getImm();
    const int64_t Min = IsSigned ? -(int64_t(1) << (Bits - 1)) : 0;
    const int64_t Max = IsSigned ? (int64_t(1) << (Bits - 1)) - 1
                                 : (int64_t(1) << Bits) - 1;
    if (Value < Min || Value > Max) {
      error(MI, "immediate is out of range");
      return;
    }
    emit8(Out, Family | *Reg);
    emit8(Out, Value);
    if (Bits == 16)
      emit8(Out, Value >> 8);
  }

  void emitColdImmediate(const MCInst &MI, SmallVectorImpl<char> &Out,
                         unsigned Family, unsigned Bits, bool IsSigned) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isImm()) {
      error(MI, "expected cold register and immediate operands");
      return;
    }
    const std::optional<unsigned> Reg = coldRegIndex(MI.getOperand(0).getReg());
    if (!Reg) {
      error(MI, "expected cold register r0-r3");
      return;
    }
    const int64_t Value = MI.getOperand(1).getImm();
    const int64_t Min = IsSigned ? -(int64_t(1) << (Bits - 1)) : 0;
    const int64_t Max = IsSigned ? (int64_t(1) << (Bits - 1)) - 1
                                 : (int64_t(1) << Bits) - 1;
    if (Value < Min || Value > Max) {
      error(MI, "immediate is out of range");
      return;
    }
    emit8(Out, 0xf0);
    emit8(Out, Family | *Reg);
    emit8(Out, Value);
    if (Bits == 16)
      emit8(Out, Value >> 8);
  }

  void emitStackU8(const MCInst &MI, SmallVectorImpl<char> &Out,
                   unsigned Family, bool IsStore = false) const {
    if (MI.getNumOperands() != 2) {
      error(MI, "expected stack register and unsigned 8-bit operand");
      return;
    }
    const MCOperand &RegOperand = MI.getOperand(IsStore ? 1 : 0);
    const MCOperand &ValueOperand = MI.getOperand(IsStore ? 0 : 1);
    if (!RegOperand.isReg() || !ValueOperand.isImm()) {
      error(MI, "expected stack register and unsigned 8-bit operand");
      return;
    }
    const std::optional<unsigned> Reg = stackRegIndex(RegOperand.getReg());
    const int64_t Value = ValueOperand.getImm();
    if (!Reg || Value < 0 || Value > 255) {
      error(MI, "expected full register r0-r7 and unsigned 8-bit operand");
      return;
    }
    emit8(Out, 0xf0);
    emit8(Out, Family | *Reg);
    emit8(Out, Value);
  }

  void emitCompactStackStore(const MCInst &MI,
                             SmallVectorImpl<char> &Out) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isImm() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected unsigned 4-bit stack offset and compact register");
      return;
    }
    const auto Reg = compactRegIndex(MI.getOperand(1).getReg());
    const int64_t Offset = MI.getOperand(0).getImm();
    if (!Reg || Offset < 0 || Offset > 15) {
      error(MI, "expected compact register c0-c3 and unsigned 4-bit offset");
      return;
    }
    emit8(Out, 0xf1);
    emit8(Out, 0x30 + 4 * unsigned(Offset) + *Reg);
  }

  void emitCompactStackLoad(const MCInst &MI,
                            SmallVectorImpl<char> &Out) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isImm()) {
      error(MI, "expected compact register and unsigned 4-bit stack offset");
      return;
    }
    const auto Reg = compactRegIndex(MI.getOperand(0).getReg());
    const int64_t Offset = MI.getOperand(1).getImm();
    if (!Reg || Offset < 0 || Offset > 15) {
      error(MI, "expected compact register c0-c3 and unsigned 4-bit offset");
      return;
    }
    emit8(Out, 0xf3);
    emit8(Out, 0x40 + 4 * unsigned(Offset) + *Reg);
  }

  void emitCompactStackWord(const MCInst &MI, SmallVectorImpl<char> &Out,
                            bool IsStore) const {
    if (MI.getNumOperands() != 2)
      return error(MI, "expected compact register and unsigned 4-bit stack offset");
    const MCOperand &RegOperand = MI.getOperand(IsStore ? 1 : 0);
    const MCOperand &ValueOperand = MI.getOperand(IsStore ? 0 : 1);
    if (!RegOperand.isReg() || !ValueOperand.isImm())
      return error(MI, "expected compact register and unsigned 4-bit stack offset");
    const auto Reg = compactRegIndex(RegOperand.getReg());
    const int64_t Offset = ValueOperand.getImm();
    if (!Reg || Offset < 0 || Offset > 15)
      return error(MI, "expected compact register c0-c3 and unsigned 4-bit offset");
    emit8(Out, 0xf4);
    emit8(Out, (IsStore ? 0x40 : 0) + 4 * unsigned(Offset) + *Reg);
  }

  void emitF1FullReg(const MCInst &MI, SmallVectorImpl<char> &Out,
                     unsigned SecondaryBase) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isReg()) {
      error(MI, "expected one full register operand");
      return;
    }
    const auto Reg = stackRegIndex(MI.getOperand(0).getReg());
    if (!Reg) {
      error(MI, "expected full register r0-r7");
      return;
    }
    emit8(Out, 0xf1);
    emit8(Out, SecondaryBase + *Reg);
  }

  void emitF4FullReg(const MCInst &MI, SmallVectorImpl<char> &Out,
                     unsigned SecondaryBase) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isReg()) {
      error(MI, "expected one full register operand");
      return;
    }
    const auto Reg = stackRegIndex(MI.getOperand(0).getReg());
    if (!Reg) {
      error(MI, "expected full register r0-r7");
      return;
    }
    emit8(Out, 0xf4);
    emit8(Out, SecondaryBase + *Reg);
  }

  void emitF6FullReg(const MCInst &MI, SmallVectorImpl<char> &Out,
                     unsigned SecondaryBase) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isReg()) {
      error(MI, "expected one full register operand");
      return;
    }
    const auto Reg = stackRegIndex(MI.getOperand(0).getReg());
    if (!Reg) {
      error(MI, "expected full register r0-r7");
      return;
    }
    emit8(Out, 0xf6);
    emit8(Out, SecondaryBase + *Reg);
  }

  void emitCSet(const MCInst &MI, SmallVectorImpl<char> &Out,
                unsigned SecondaryBase) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isReg()) {
      error(MI, "expected one full register operand");
      return;
    }
    const auto Reg = stackRegIndex(MI.getOperand(0).getReg());
    if (!Reg) {
      error(MI, "expected full register r0-r7");
      return;
    }
    emit8(Out, 0xf8);
    emit8(Out, SecondaryBase + *Reg);
  }

  void emitF6Multiply(const MCInst &MI, SmallVectorImpl<char> &Out) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected compact register operands");
      return;
    }
    const auto D = compactRegIndex(MI.getOperand(0).getReg());
    const auto S = compactRegIndex(MI.getOperand(1).getReg());
    if (!D || !S) {
      error(MI, "expected compact register c0-c3");
      return;
    }
    emit8(Out, 0xf6);
    emit8(Out, 0x30 + 4 * *D + *S);
  }

  void emitAbsoluteData(const MCInst &MI, SmallVectorImpl<char> &Out,
                        SmallVectorImpl<MCFixup> &Fixups, unsigned Family,
                        bool IsStore) const {
    if (MI.getNumOperands() != 2) {
      error(MI, "expected absolute data address and full register operands");
      return;
    }
    const MCOperand &Address = MI.getOperand(IsStore ? 0 : 1);
    const MCOperand &Register = MI.getOperand(IsStore ? 1 : 0);
    if (!Register.isReg() || (!Address.isImm() && !Address.isExpr())) {
      error(MI, "expected absolute data address and full register operands");
      return;
    }
    const std::optional<unsigned> Reg = stackRegIndex(Register.getReg());
    if (!Reg) {
      error(MI, "expected full register r0-r7");
      return;
    }
    emit8(Out, 0xf0);
    emit8(Out, Family | *Reg);
    if (Address.isImm()) {
      if (!isUInt<16>(Address.getImm()))
        error(MI, "absolute address is out of unsigned 16-bit range");
      emit16(Out, Address.getImm());
      return;
    }
    Fixups.push_back(
        MCFixup::create(2, Address.getExpr(), AVM::fixup_avm_data16));
    emit16(Out, 0);
  }

public:
  explicit AVMMCCodeEmitter(MCContext &Ctx) : Ctx(Ctx) {}

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &Out,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &) const override {
    switch (MI.getOpcode()) {
    case AVM::MOV: emitCompactMatrix(MI, Out, 0x00); return;
    case AVM::MOV_RR: emitFullMove(MI, Out); return;
    case AVM::CMP_RR: emitFullCompare(MI, Out); return;
    case AVM::ADD_RR: emitFullArithmetic(MI, Out, 0x00); return;
    case AVM::SUB_RR: emitFullArithmetic(MI, Out, 0x30); return;
    case AVM::AND_RR: emitFullBitwise(MI, Out, 0); return;
    case AVM::OR_RR: emitFullBitwise(MI, Out, 1); return;
    case AVM::XOR_RR: emitFullBitwise(MI, Out, 2); return;
    case AVM::ZEXT8: emitF1FullReg(MI, Out, 0x70); return;
    case AVM::SWAP8: emitF1FullReg(MI, Out, 0x78); return;
    case AVM::GETSP: emitF1FullReg(MI, Out, 0x80); return;
    case AVM::SETSP: emitF1FullReg(MI, Out, 0x88); return;
    case AVM::LSL16_1: emitF4FullReg(MI, Out, 0x80); return;
    case AVM::LSR16_1: emitF4FullReg(MI, Out, 0x88); return;
    case AVM::ASR16_1: emitF4FullReg(MI, Out, 0x90); return;
    case AVM::NOT16: emitF4FullReg(MI, Out, 0x98); return;
    case AVM::TST8: emitF4FullReg(MI, Out, 0xa0); return;
    case AVM::INC16: emitF4FullReg(MI, Out, 0xa8); return;
    case AVM::DEC16: emitF4FullReg(MI, Out, 0xb0); return;
    case AVM::BSWAP16: emitF6FullReg(MI, Out, 0x20); return;
    case AVM::TST16: emitF6FullReg(MI, Out, 0x28); return;
    case AVM::MUL8: emitF6Multiply(MI, Out); return;
    case AVM::SEXT8: emitF6FullReg(MI, Out, 0x40); return;
    case AVM::NEG16: emitF6FullReg(MI, Out, 0x48); return;
    case AVM::CSET_EQ: emitCSet(MI, Out, 0x00); return;
    case AVM::CSET_NE: emitCSet(MI, Out, 0x08); return;
    case AVM::CSET_ULT: emitCSet(MI, Out, 0x10); return;
    case AVM::CSET_UGE: emitCSet(MI, Out, 0x18); return;
    case AVM::CSET_SLT: emitCSet(MI, Out, 0x20); return;
    case AVM::CSET_SGE: emitCSet(MI, Out, 0x28); return;
    case AVM::STSP8_COMPACT: emitCompactStackStore(MI, Out); return;
    case AVM::LDSP8U_COMPACT: emitCompactStackLoad(MI, Out); return;
    case AVM::ADD: emitCompactMatrix(MI, Out, 0x10); return;
    case AVM::SUB: emitCompactMatrix(MI, Out, 0x20); return;
    case AVM::CMP: emitCompactMatrix(MI, Out, 0x30); return;
    case AVM::LD8U: emitCompactMatrix(MI, Out, 0x40); return;
    case AVM::ST8: emitCompactMatrix(MI, Out, 0x50); return;
    case AVM::F3ST8: emitF3Store(MI, Out); return;
    case AVM::F6ST8_POST: emitF6PostStore(MI, Out); return;
    case AVM::F5LD8U: emitF5Memory(MI, Out, 0x30, false); return;
    case AVM::F5LD16: emitF5Memory(MI, Out, 0x40, false); return;
    case AVM::F5ST16: emitF5Memory(MI, Out, 0x50, true); return;
    case AVM::F7LD8U_POST: emitF7Memory(MI, Out, 0x00, false); return;
    case AVM::F7LD16_POST: emitF7Memory(MI, Out, 0x20, false); return;
    case AVM::F7ST16_POST: emitF7Memory(MI, Out, 0x40, true); return;
    case AVM::ADD32: emitF7PairArithmetic(MI, Out, 0x60); return;
    case AVM::SUB32: emitF7PairArithmetic(MI, Out, 0x70); return;
    case AVM::LSR32_1: emitF7PairUnary(MI, Out, 0x80); return;
    case AVM::ASR32_1: emitF7PairUnary(MI, Out, 0x84); return;
    case AVM::BOOL: emitF7Bool(MI, Out); return;
    case AVM::MULU8W: emitF3Multiply(MI, Out, 0x10); return;
    case AVM::MULS8W: emitF3Multiply(MI, Out, 0x20); return;
    case AVM::MULSU8W: emitF3Multiply(MI, Out, 0x30); return;
    case AVM::LD16: emitCompactMatrix(MI, Out, 0x60); return;
    case AVM::ST16: emitCompactMatrix(MI, Out, 0x70); return;
    case AVM::AND: emitCompactMatrix(MI, Out, 0x80); return;
    case AVM::OR: emitCompactMatrix(MI, Out, 0x90); return;
    case AVM::XOR: emitCompactMatrix(MI, Out, 0xa0); return;
    case AVM::PUSH16: emitStackReg(MI, Out, 0xb0); return;
    case AVM::POP16: emitStackReg(MI, Out, 0xb8); return;
    case AVM::LDI8: emitCompactImmediate(MI, Out, 0xc0, 8, false); return;
    case AVM::LDI16: emitCompactImmediate(MI, Out, 0xc4, 16, false); return;
    case AVM::ADDIS8: emitCompactImmediate(MI, Out, 0xc8, 8, true); return;
    case AVM::CMPIS8: emitCompactImmediate(MI, Out, 0xcc, 8, true); return;
    case AVM::COLDLDI8: emitColdImmediate(MI, Out, 0x00, 8, false); return;
    case AVM::COLDLDI16: emitColdImmediate(MI, Out, 0x04, 16, false); return;
    case AVM::COLDADDIS8: emitColdImmediate(MI, Out, 0x08, 8, true); return;
    case AVM::COLDCMPIS8: emitColdImmediate(MI, Out, 0x0c, 8, true); return;
    case AVM::LEASP: emitStackU8(MI, Out, 0x10); return;
    case AVM::LDSP8U: emitStackU8(MI, Out, 0x18); return;
    case AVM::LDSP8S: emitStackU8(MI, Out, 0x20); return;
    case AVM::STSP8: emitStackU8(MI, Out, 0x28, true); return;
    case AVM::LDSP16: emitStackU8(MI, Out, 0x30); return;
    case AVM::STSP16: emitStackU8(MI, Out, 0x38, true); return;
    case AVM::LDSP16_COMPACT: emitCompactStackWord(MI, Out, false); return;
    case AVM::STSP16_COMPACT: emitCompactStackWord(MI, Out, true); return;
    case AVM::LDM8U: emitAbsoluteData(MI, Out, Fixups, 0x40, false); return;
    case AVM::STM8: emitAbsoluteData(MI, Out, Fixups, 0x48, true); return;
    case AVM::LDM16: emitAbsoluteData(MI, Out, Fixups, 0x50, false); return;
    case AVM::STM16: emitAbsoluteData(MI, Out, Fixups, 0x58, true); return;
    case AVM::LDP8U: emitProgramLoad(MI, Out, 0x60, false); return;
    case AVM::LDP8S: emitProgramLoad(MI, Out, 0x61, false); return;
    case AVM::LDP16: emitProgramLoad(MI, Out, 0x62, false); return;
    case AVM::LDP24: emitProgramLoad(MI, Out, 0x63, true); return;
    case AVM::LDP32: emitProgramLoad(MI, Out, 0x64, true); return;
    case AVM::LDP8U_POST: emitProgramLoad(MI, Out, 0x65, false); return;
    case AVM::LDP16_POST: emitProgramLoad(MI, Out, 0x66, false); return;
    case AVM::LDP24_POST: emitProgramLoad(MI, Out, 0x67, true); return;
    case AVM::LDP32_POST: emitProgramLoad(MI, Out, 0x68, true); return;
    case AVM::CMP32: emitCold32(MI, Out, 0x69, true, false); return;
    case AVM::LD32: emitCold32(MI, Out, 0x6a, false, false); return;
    case AVM::ST32: emitCold32(MI, Out, 0x6b, false, true); return;
    case AVM::GPLD8U: emitGeneralPointer(MI, Out, 0x6c, false, false, false); return;
    case AVM::GPLD16: emitGeneralPointer(MI, Out, 0x6c, true, false, false); return;
    case AVM::GPLD8U_POST: emitGeneralPointer(MI, Out, 0x6c, false, true, false); return;
    case AVM::GPLD16_POST: emitGeneralPointer(MI, Out, 0x6c, true, true, false); return;
    case AVM::GPST8: emitGeneralPointer(MI, Out, 0x6d, false, false, true); return;
    case AVM::GPST16: emitGeneralPointer(MI, Out, 0x6d, true, false, true); return;
    case AVM::GPST8_POST: emitGeneralPointer(MI, Out, 0x6d, false, true, true); return;
    case AVM::GPST16_POST: emitGeneralPointer(MI, Out, 0x6d, true, true, true); return;
    case AVM::BREQ: emitRel8(MI, Out, Fixups, 0xd0); return;
    case AVM::BRNE: emitRel8(MI, Out, Fixups, 0xd1); return;
    case AVM::BRULT: emitRel8(MI, Out, Fixups, 0xd2); return;
    case AVM::BRSLT: emitRel8(MI, Out, Fixups, 0xd3); return;
    case AVM::JMP: emitRel8(MI, Out, Fixups, 0xd4); return;
    case AVM::CALL: emitSigned8(MI, Out, 0xd5); return;
    case AVM::ADJSP: emitSigned8(MI, Out, 0xd6); return;
    case AVM::SYS: emitService(MI, Out); return;
    case AVM::JMP16: emitRel16(MI, Out, Fixups, 0xe0); return;
    case AVM::CALL16: emitRel16(MI, Out, Fixups, 0xe1); return;
    case AVM::JMPF:
      emit8(Out, 0xe2);
      emitFarTarget(MI, Out, Fixups);
      return;
    case AVM::CALLF:
      emit8(Out, 0xe3);
      emitFarTarget(MI, Out, Fixups);
      return;
    case AVM::JMPP: emitProgramPair(MI, Out, 0xe4); return;
    case AVM::CALLP: emitProgramPair(MI, Out, 0xe8); return;
    case AVM::RET: emit8(Out, 0xef); return;
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
