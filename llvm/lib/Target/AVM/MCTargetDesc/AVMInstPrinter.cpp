#include "AVMInstPrinter.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"

using namespace llvm;

std::pair<const char *, uint64_t>
AVMInstPrinter::getMnemonic(const MCInst &MI) const {
  switch (MI.getOpcode()) {
  case AVM::MOV: return {"mov", 0};
  case AVM::MOV32:
  case AVM::MOV32_F2: return {"mov32", 0};
  case AVM::FADD: return {"fadd", 0};
  case AVM::FSUB: return {"fsub", 0};
  case AVM::FMUL: return {"fmul", 0};
  case AVM::FDIV: return {"fdiv", 0};
  case AVM::FMIN: return {"fmin", 0};
  case AVM::FMAX: return {"fmax", 0};
  case AVM::FNEG: return {"fneg", 0};
  case AVM::FABS: return {"fabs", 0};
  case AVM::FSQRT: return {"fsqrt", 0};
  case AVM::FTRUNC: return {"ftrunc", 0};
  case AVM::FFLOOR: return {"ffloor", 0};
  case AVM::FCEIL: return {"fceil", 0};
  case AVM::FROUND: return {"fround", 0};
  case AVM::S16TOF: return {"s16tof", 0};
  case AVM::U16TOF: return {"u16tof", 0};
  case AVM::FTOS16: return {"ftos16", 0};
  case AVM::FTOU16: return {"ftou16", 0};
  case AVM::S32TOF: return {"s32tof", 0};
  case AVM::U32TOF: return {"u32tof", 0};
  case AVM::FTOS32: return {"ftos32", 0};
  case AVM::FTOU32: return {"ftou32", 0};
  case AVM::FCMP: return {"fcmp", 0};
  case AVM::FCLASS: return {"fclass", 0};
  case AVM::MOV_RR: return {"mov", 0};
  case AVM::CMP_RR: return {"cmp", 0};
  case AVM::ADD_RR: return {"add", 0};
  case AVM::SUB_RR: return {"sub", 0};
  case AVM::AND_RR: return {"and", 0};
  case AVM::OR_RR: return {"or", 0};
  case AVM::XOR_RR: return {"xor", 0};
  case AVM::MUL16: return {"mul16", 0};
  case AVM::UDIV16: return {"udiv16", 0};
  case AVM::UREM16: return {"urem16", 0};
  case AVM::SDIV16: return {"sdiv16", 0};
  case AVM::SREM16: return {"srem16", 0};
  case AVM::ZEXT8: return {"zext8", 0};
  case AVM::SWAP8: return {"swap8", 0};
  case AVM::GETSP: return {"getsp", 0};
  case AVM::SETSP: return {"setsp", 0};
  case AVM::LSL16_1: return {"lsl16.1", 0};
  case AVM::LSR16_1: return {"lsr16.1", 0};
  case AVM::ASR16_1: return {"asr16.1", 0};
  case AVM::SHL16V: return {"shl16v", 0};
  case AVM::LSR16V: return {"lsr16v", 0};
  case AVM::ASR16V: return {"asr16v", 0};
  case AVM::LSL16I: return {"lsl16i", 0};
  case AVM::LSR16I: return {"lsr16i", 0};
  case AVM::ASR16I: return {"asr16i", 0};
  case AVM::NOT16: return {"not16", 0};
  case AVM::TST8: return {"tst8", 0};
  case AVM::INC16: return {"inc16", 0};
  case AVM::DEC16: return {"dec16", 0};
  case AVM::BSWAP16: return {"bswap16", 0};
  case AVM::TST16: return {"tst16", 0};
  case AVM::MUL8: return {"mul8", 0};
  case AVM::SEXT8: return {"sext8", 0};
  case AVM::NEG16: return {"neg16", 0};
  case AVM::CSET_EQ: return {"cset.eq", 0};
  case AVM::CSET_NE: return {"cset.ne", 0};
  case AVM::CSET_ULT: return {"cset.ult", 0};
  case AVM::CSET_UGE: return {"cset.uge", 0};
  case AVM::CSET_SLT: return {"cset.slt", 0};
  case AVM::CSET_SGE: return {"cset.sge", 0};
  case AVM::CMOV_EQ: return {"cmov.eq", 0};
  case AVM::CMOV_NE: return {"cmov.ne", 0};
  case AVM::CMOV_ULT: return {"cmov.ult", 0};
  case AVM::CMOV_UGE: return {"cmov.uge", 0};
  case AVM::CMOV_SLT: return {"cmov.slt", 0};
  case AVM::CMOV_SGE: return {"cmov.sge", 0};
  case AVM::ADD: return {"add", 0};
  case AVM::SUB: return {"sub", 0};
  case AVM::CMP: return {"cmp", 0};
  case AVM::LD8U: return {"ld8u", 0};
  case AVM::ST8: return {"st8", 0};
  case AVM::F3ST8: return {"st8", 0};
  case AVM::F6ST8_POST: return {"st8", 0};
  case AVM::F5LD8U: return {"ld8u", 0};
  case AVM::F5LD16: return {"ld16", 0};
  case AVM::F5ST16: return {"st16", 0};
  case AVM::F7LD8U_POST: return {"ld8u", 0};
  case AVM::F7LD16_POST: return {"ld16", 0};
  case AVM::F7ST16_POST: return {"st16", 0};
  case AVM::DPLD8U: return {"ld8u", 0};
  case AVM::DPLD16: return {"ld16", 0};
  case AVM::DPST8: return {"st8", 0};
  case AVM::DPST16: return {"st16", 0};
  case AVM::ADD32: return {"add32", 0};
  case AVM::SUB32: return {"sub32", 0};
  case AVM::LSR32_1: return {"lsr32.1", 0};
  case AVM::ASR32_1: return {"asr32.1", 0};
  case AVM::BOOL: return {"bool", 0};
  case AVM::MULU8W: return {"mulu8.w", 0};
  case AVM::MULS8W: return {"muls8.w", 0};
  case AVM::MULSU8W: return {"mulsu8.w", 0};
  case AVM::LD16: return {"ld16", 0};
  case AVM::ST16: return {"st16", 0};
  case AVM::AND: return {"and", 0};
  case AVM::OR: return {"or", 0};
  case AVM::XOR: return {"xor", 0};
  case AVM::PUSH16: return {"push16", 0};
  case AVM::POP16: return {"pop16", 0};
  case AVM::JMPF:
    return {"jmpf", 0};
  case AVM::CALLF:
    return {"callf", 0};
  case AVM::JMP16: return {"jmp16", 0};
  case AVM::CALL16: return {"call16", 0};
  case AVM::JMPP: return {"jmpp", 0};
  case AVM::CALLP: return {"callp", 0};
  case AVM::RET: return {"ret", 0};
  case AVM::BREQ8: return {"breq8", 0};
  case AVM::BRNE8: return {"brne8", 0};
  case AVM::BRULT8: return {"brult8", 0};
  case AVM::BRSLT8: return {"brslt8", 0};
  case AVM::BRUGE8: return {"bruge8", 0};
  case AVM::BRSGE8: return {"brsge8", 0};
  case AVM::BREQ16: return {"breq16", 0};
  case AVM::BRNE16: return {"brne16", 0};
  case AVM::BRULT16: return {"brult16", 0};
  case AVM::BRUGE16: return {"bruge16", 0};
  case AVM::BRSLT16: return {"brslt16", 0};
  case AVM::BRSGE16: return {"brsge16", 0};
  case AVM::RELAX_JMP: return {"jmp", 0};
  case AVM::RELAX_CALL: return {"call", 0};
  case AVM::RELAX_BR_EQ: return {"breq", 0};
  case AVM::RELAX_BR_NE: return {"brne", 0};
  case AVM::RELAX_BR_ULT: return {"brult", 0};
  case AVM::RELAX_BR_UGE: return {"bruge", 0};
  case AVM::RELAX_BR_SLT: return {"brslt", 0};
  case AVM::RELAX_BR_SGE: return {"brsge", 0};
  case AVM::JMP8: return {"jmp8", 0};
  case AVM::CALL8: return {"call8", 0};
  case AVM::ADJSP: return {"adjsp", 0};
  case AVM::SYS: return {"sys", 0};
  case AVM::LDI8: return {"ldi8", 0};
  case AVM::LDI16: return {"ldi16", 0};
  case AVM::ADDIS8: return {"addi.s8", 0};
  case AVM::CMPIS8: return {"cmpi.s8", 0};
  case AVM::COLDLDI8: return {"ldi8", 0};
  case AVM::COLDLDI16: return {"ldi16", 0};
  case AVM::COLDADDIS8: return {"addi.s8", 0};
  case AVM::COLDCMPIS8: return {"cmpi.s8", 0};
  case AVM::LEASP: return {"leasp", 0};
  case AVM::LDSP8U: return {"ldsp8u", 0};
  case AVM::LDSP8U_COMPACT: return {"ldsp8u", 0};
  case AVM::LDSP8S: return {"ldsp8s", 0};
  case AVM::STSP8: return {"stsp8", 0};
  case AVM::STSP8_COMPACT: return {"stsp8", 0};
  case AVM::LDSP16: return {"ldsp16", 0};
  case AVM::STSP16: return {"stsp16", 0};
  case AVM::LDSP16_COMPACT: return {"ldsp16", 0};
  case AVM::STSP16_COMPACT: return {"stsp16", 0};
  case AVM::LDM8U: return {"ldm8u", 0};
  case AVM::STM8: return {"stm8", 0};
  case AVM::LDM16: return {"ldm16", 0};
  case AVM::STM16: return {"stm16", 0};
  case AVM::LDP8U:
  case AVM::LDP8U_POST: return {"ldp8u", 0};
  case AVM::LDP8S: return {"ldp8s", 0};
  case AVM::LDP16:
  case AVM::LDP16_POST: return {"ldp16", 0};
  case AVM::LDP24:
  case AVM::LDP24_POST: return {"ldp24", 0};
  case AVM::LDP32:
  case AVM::LDP32_POST: return {"ldp32", 0};
  case AVM::CMP32: return {"cmp32", 0};
  case AVM::LD32: return {"ld32", 0};
  case AVM::ST32: return {"st32", 0};
  case AVM::GPLD8U_POST: return {"ld8u", 0};
  case AVM::GPLD16_POST: return {"ld16", 0};
  case AVM::GPST8_POST: return {"st8", 0};
  case AVM::GPST16_POST: return {"st16", 0};
  case AVM::PROGPTR: return {".progptr", 0};
  default:
    return {"<unknown>", 0};
  }
}

void AVMInstPrinter::printCompactReg(MCRegister Reg, raw_ostream &OS) const {
  switch (Reg.id()) {
  case AVM::R4: OS << "r4"; return;
  case AVM::R5: OS << "r5"; return;
  case AVM::R6: OS << "r6"; return;
  case AVM::R7: OS << "r7"; return;
  default: OS << "<bad-compact-reg>"; return;
  }
}

void AVMInstPrinter::printFullReg(MCRegister Reg, raw_ostream &OS) const {
  OS << getRegisterName(Reg);
}

StringRef AVMInstPrinter::getRegisterName(MCRegister Reg) {
  switch (Reg.id()) {
  case AVM::R0:
    return "r0";
  case AVM::R1:
    return "r1";
  case AVM::R2:
    return "r2";
  case AVM::R3:
    return "r3";
  case AVM::R4:
    return "r4";
  case AVM::R5:
    return "r5";
  case AVM::R6:
    return "r6";
  case AVM::R7:
    return "r7";
  case AVM::R0R1:
    return "q0";
  case AVM::R2R3:
    return "q1";
  case AVM::R4R5:
    return "q2";
  case AVM::R6R7:
    return "q3";
  case AVM::SP:
    return "sp";
  case AVM::PC:
    return "pc";
  case AVM::CC:
    return "cc";
  default:
    return "<bad-reg>";
  }
}

void AVMInstPrinter::printRegName(raw_ostream &OS, MCRegister Reg) {
  printFullReg(Reg, OS);
}

void AVMInstPrinter::printOperand(const MCOperand &Op, raw_ostream &OS) const {
  if (Op.isReg()) {
    printFullReg(Op.getReg(), OS);
    return;
  }
  if (Op.isImm()) {
    OS << formatImm(Op.getImm());
    return;
  }
  if (Op.isExpr()) {
    MAI.printExpr(OS, *Op.getExpr());
    return;
  }
  llvm_unreachable("unsupported AVM operand");
}

void AVMInstPrinter::printInst(const MCInst *MI, uint64_t, StringRef Annot,
                               const MCSubtargetInfo &, raw_ostream &OS) {
  if (MI->getOpcode() == AVM::MOV && MI->getNumOperands() == 2 &&
      MI->getOperand(0).isReg() && MI->getOperand(1).isReg() &&
      MI->getOperand(0).getReg() == AVM::R4 &&
      MI->getOperand(1).getReg() == AVM::R4) {
    OS << "nop";
    printAnnotation(OS, Annot);
    return;
  }
  OS << getMnemonic(*MI).first;
  switch (MI->getOpcode()) {
  case AVM::MOV32:
  case AVM::MOV32_F2:
  case AVM::FADD:
  case AVM::FSUB:
  case AVM::FMUL:
  case AVM::FDIV:
  case AVM::FMIN:
  case AVM::FMAX:
  case AVM::S16TOF:
  case AVM::U16TOF:
  case AVM::FTOS16:
  case AVM::FTOU16:
  case AVM::S32TOF:
  case AVM::U32TOF:
  case AVM::FTOS32:
  case AVM::FTOU32:
  case AVM::FCLASS:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::FCMP:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    OS << ", ";
    printFullReg(MI->getOperand(2).getReg(), OS);
    break;
  case AVM::FNEG:
  case AVM::FABS:
  case AVM::FSQRT:
  case AVM::FTRUNC:
  case AVM::FFLOOR:
  case AVM::FCEIL:
  case AVM::FROUND:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    break;
  case AVM::LD8U:
  case AVM::LD16:
    OS << '\t';
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << ", [";
    printCompactReg(MI->getOperand(1).getReg(), OS);
    OS << ']';
    break;
  case AVM::ST8:
  case AVM::ST16:
    OS << "\t[";
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << "], ";
    printCompactReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::F3ST8:
    OS << "\t[";
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << "], ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::F5LD8U:
  case AVM::F5LD16:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", [";
    printCompactReg(MI->getOperand(1).getReg(), OS);
    OS << ']';
    break;
  case AVM::F5ST16:
    OS << "\t[";
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << "], ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::F7LD8U_POST:
  case AVM::F7LD16_POST:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", [";
    printCompactReg(MI->getOperand(1).getReg(), OS);
    OS << "+]";
    break;
  case AVM::F7ST16_POST:
    OS << "\t[";
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << "+], ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::ADD32:
  case AVM::SUB32:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::LSR32_1:
  case AVM::ASR32_1:
  case AVM::BOOL:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    break;
  case AVM::CSET_EQ:
  case AVM::CSET_NE:
  case AVM::CSET_ULT:
  case AVM::CSET_UGE:
  case AVM::CSET_SLT:
  case AVM::CSET_SGE:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    break;
  case AVM::CMOV_EQ:
  case AVM::CMOV_NE:
  case AVM::CMOV_ULT:
  case AVM::CMOV_UGE:
  case AVM::CMOV_SLT:
  case AVM::CMOV_SGE:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::MULU8W:
  case AVM::MULS8W:
  case AVM::MULSU8W:
  case AVM::MUL8:
    OS << '\t';
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printCompactReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::MOV:
  case AVM::ADD:
  case AVM::SUB:
  case AVM::CMP:
  case AVM::AND:
  case AVM::OR:
  case AVM::XOR:
  case AVM::SHL16V:
  case AVM::LSR16V:
  case AVM::ASR16V:
    OS << '\t';
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printCompactReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::LSL16I:
  case AVM::LSR16I:
  case AVM::ASR16I:
    OS << '\t';
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << ", " << formatImm(MI->getOperand(1).getImm());
    break;
  case AVM::MOV_RR:
  case AVM::CMP_RR:
  case AVM::ADD_RR:
  case AVM::SUB_RR:
  case AVM::AND_RR:
  case AVM::OR_RR:
  case AVM::XOR_RR:
  case AVM::MUL16:
  case AVM::UDIV16:
  case AVM::UREM16:
  case AVM::SDIV16:
  case AVM::SREM16:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::ZEXT8:
  case AVM::SWAP8:
  case AVM::GETSP:
  case AVM::SETSP:
  case AVM::LSL16_1:
  case AVM::LSR16_1:
  case AVM::ASR16_1:
  case AVM::NOT16:
  case AVM::TST8:
  case AVM::INC16:
  case AVM::DEC16:
  case AVM::BSWAP16:
  case AVM::TST16:
  case AVM::SEXT8:
  case AVM::NEG16:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    break;
  case AVM::PUSH16:
  case AVM::POP16:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    break;
  case AVM::LDI8:
  case AVM::LDI16:
  case AVM::ADDIS8:
  case AVM::CMPIS8:
    OS << '\t';
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printOperand(MI->getOperand(1), OS);
    break;
  case AVM::COLDLDI8:
  case AVM::COLDLDI16:
  case AVM::COLDADDIS8:
  case AVM::COLDCMPIS8:
  case AVM::LEASP:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printOperand(MI->getOperand(1), OS);
    break;
  case AVM::LDSP8U:
  case AVM::LDSP8S:
  case AVM::LDSP16:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", [sp+" << formatImm(MI->getOperand(1).getImm()) << ']';
    break;
  case AVM::LDSP16_COMPACT:
    OS << '\t';
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << ", [sp+" << formatImm(MI->getOperand(1).getImm()) << ']';
    break;
  case AVM::LDSP8U_COMPACT:
    OS << '\t';
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << ", [sp+" << formatImm(MI->getOperand(1).getImm()) << ']';
    break;
  case AVM::STSP8:
  case AVM::STSP16:
    OS << "\t[sp+" << formatImm(MI->getOperand(0).getImm()) << "], ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::STSP16_COMPACT:
    OS << "\t[sp+" << formatImm(MI->getOperand(0).getImm()) << "], ";
    printCompactReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::STSP8_COMPACT:
    OS << "\t[sp+" << formatImm(MI->getOperand(0).getImm()) << "], ";
    printCompactReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::LDM8U:
  case AVM::LDM16:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", [";
    printOperand(MI->getOperand(1), OS);
    OS << ']';
    break;
  case AVM::STM8:
  case AVM::STM16:
    OS << "\t[";
    printOperand(MI->getOperand(0), OS);
    OS << "], ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::LDP8U:
  case AVM::LDP8S:
  case AVM::LDP16:
  case AVM::LDP24:
  case AVM::LDP32:
  case AVM::LDP8U_POST:
  case AVM::LDP16_POST:
  case AVM::LDP24_POST:
  case AVM::LDP32_POST:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", [";
    printFullReg(MI->getOperand(1).getReg(), OS);
    if (MI->getOpcode() == AVM::LDP8U_POST ||
        MI->getOpcode() == AVM::LDP16_POST ||
        MI->getOpcode() == AVM::LDP24_POST ||
        MI->getOpcode() == AVM::LDP32_POST)
      OS << '+';
    OS << ']';
    break;
  case AVM::CMP32:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::LD32:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", [";
    printFullReg(MI->getOperand(1).getReg(), OS);
    OS << ']';
    break;
  case AVM::ST32:
    OS << "\t[";
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << "], ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::GPLD8U_POST:
  case AVM::GPLD16_POST:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", [";
    printFullReg(MI->getOperand(1).getReg(), OS);
    OS << '+';
    OS << ']';
    break;
  case AVM::GPST8_POST:
  case AVM::GPST16_POST:
    OS << "\t[";
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << '+';
    OS << "], ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::DPLD8U:
  case AVM::DPLD16:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", [";
    printFullReg(MI->getOperand(1).getReg(), OS);
    if (MI->getOperand(2).getImm() >= 0)
      OS << '+';
    OS << MI->getOperand(2).getImm() << ']';
    break;
  case AVM::DPST8:
  case AVM::DPST16:
    OS << "\t[";
    printFullReg(MI->getOperand(0).getReg(), OS);
    if (MI->getOperand(1).getImm() >= 0)
      OS << '+';
    OS << MI->getOperand(1).getImm() << "], ";
    printFullReg(MI->getOperand(2).getReg(), OS);
    break;
  case AVM::F6ST8_POST:
    OS << "\t[";
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << "+], ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::JMP16:
  case AVM::CALL16:
    OS << '\t';
    if (MI->getOperand(0).isImm())
      OS << MI->getOperand(0).getImm();
    else
      printOperand(MI->getOperand(0), OS);
    break;
  case AVM::SYS:
    OS << '\t';
    switch (MI->getOperand(0).getImm()) {
#define AVM_SYS_DEF(ID, AsmName, PseudoKind, Pseudo, IntrinsicKind,          \
                    Intrinsic, CostKind, Cost)                               \
  case ID:                                                                    \
    OS << #AsmName;                                                           \
    break;
#include "AVMSystemCalls.inc"
#undef AVM_SYS_DEF
    default: OS << MI->getOperand(0).getImm(); break;
    }
    break;
  default:
    if (MI->getNumOperands()) {
      OS << '\t';
      printOperand(MI->getOperand(0), OS);
    }
    break;
  }
  printAnnotation(OS, Annot);
}
