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
  case AVM::MOV_RR: return {"mov", 0};
  case AVM::CMP_RR: return {"cmp", 0};
  case AVM::ADD_RR: return {"add", 0};
  case AVM::SUB_RR: return {"sub", 0};
  case AVM::AND_RR: return {"and", 0};
  case AVM::OR_RR: return {"or", 0};
  case AVM::XOR_RR: return {"xor", 0};
  case AVM::MUL16: return {"mul16", 0};
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
  case AVM::BREQ: return {"breq", 0};
  case AVM::BRNE: return {"brne", 0};
  case AVM::BRULT: return {"brult", 0};
  case AVM::BRSLT: return {"brslt", 0};
  case AVM::JMP: return {"jmp", 0};
  case AVM::CALL: return {"call", 0};
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
  case AVM::GPLD8U:
  case AVM::GPLD8U_POST: return {"ld8u", 0};
  case AVM::GPLD16:
  case AVM::GPLD16_POST: return {"ld16", 0};
  case AVM::GPST8:
  case AVM::GPST8_POST: return {"st8", 0};
  case AVM::GPST16:
  case AVM::GPST16_POST: return {"st16", 0};
  default:
    return {"<unknown>", 0};
  }
}

void AVMInstPrinter::printCompactReg(MCRegister Reg, raw_ostream &OS) const {
  switch (Reg.id()) {
  case AVM::R4: OS << "c0"; return;
  case AVM::R5: OS << "c1"; return;
  case AVM::R6: OS << "c2"; return;
  case AVM::R7: OS << "c3"; return;
  default: OS << "<bad-compact-reg>"; return;
  }
}

void AVMInstPrinter::printFullReg(MCRegister Reg, raw_ostream &OS) const {
  switch (Reg.id()) {
  case AVM::R0: OS << "r0"; return;
  case AVM::R1: OS << "r1"; return;
  case AVM::R2: OS << "r2"; return;
  case AVM::R3: OS << "r3"; return;
  case AVM::R4: OS << "r4"; return;
  case AVM::R5: OS << "r5"; return;
  case AVM::R6: OS << "r6"; return;
  case AVM::R7: OS << "r7"; return;
  case AVM::R0R1: OS << "q0"; return;
  case AVM::R2R3: OS << "q1"; return;
  case AVM::R4R5: OS << "q2"; return;
  case AVM::R6R7: OS << "q3"; return;
  case AVM::SP: OS << "sp"; return;
  case AVM::PC: OS << "pc"; return;
  case AVM::CC: OS << "cc"; return;
  default: OS << "<bad-reg>"; return;
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
  case AVM::MOV_RR:
  case AVM::CMP_RR:
  case AVM::ADD_RR:
  case AVM::SUB_RR:
  case AVM::AND_RR:
  case AVM::OR_RR:
  case AVM::XOR_RR:
  case AVM::MUL16:
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
    OS << ", " << formatImm(MI->getOperand(1).getImm());
    break;
  case AVM::COLDLDI8:
  case AVM::COLDLDI16:
  case AVM::COLDADDIS8:
  case AVM::COLDCMPIS8:
  case AVM::LEASP:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", " << formatImm(MI->getOperand(1).getImm());
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
  case AVM::GPLD8U:
  case AVM::GPLD16:
  case AVM::GPLD8U_POST:
  case AVM::GPLD16_POST:
    OS << '\t';
    printFullReg(MI->getOperand(0).getReg(), OS);
    OS << ", [";
    printFullReg(MI->getOperand(1).getReg(), OS);
    if (MI->getOpcode() == AVM::GPLD8U_POST ||
        MI->getOpcode() == AVM::GPLD16_POST)
      OS << '+';
    OS << ']';
    break;
  case AVM::GPST8:
  case AVM::GPST16:
  case AVM::GPST8_POST:
  case AVM::GPST16_POST:
    OS << "\t[";
    printFullReg(MI->getOperand(0).getReg(), OS);
    if (MI->getOpcode() == AVM::GPST8_POST ||
        MI->getOpcode() == AVM::GPST16_POST)
      OS << '+';
    OS << "], ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::F6ST8_POST:
    OS << "\t[";
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << "+], ";
    printFullReg(MI->getOperand(1).getReg(), OS);
    break;
  case AVM::JMP16:
  case AVM::CALL16:
    OS << '\t' << MI->getOperand(0).getImm();
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
