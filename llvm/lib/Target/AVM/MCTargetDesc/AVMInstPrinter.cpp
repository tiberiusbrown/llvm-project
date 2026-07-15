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
  case AVM::ADD: return {"add", 0};
  case AVM::SUB: return {"sub", 0};
  case AVM::CMP: return {"cmp", 0};
  case AVM::LD8U: return {"ld8u", 0};
  case AVM::ST8: return {"st8", 0};
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
  case AVM::MOV:
  case AVM::ADD:
  case AVM::SUB:
  case AVM::CMP:
  case AVM::AND:
  case AVM::OR:
  case AVM::XOR:
    OS << '\t';
    printCompactReg(MI->getOperand(0).getReg(), OS);
    OS << ", ";
    printCompactReg(MI->getOperand(1).getReg(), OS);
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
