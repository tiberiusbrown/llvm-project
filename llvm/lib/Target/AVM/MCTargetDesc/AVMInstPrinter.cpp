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
  case AVM::JMPF:
    return {"jmpf", 0};
  case AVM::CALLF:
    return {"callf", 0};
  default:
    return {"<unknown>", 0};
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
  if (MI->getNumOperands()) {
    OS << '\t';
    printOperand(MI->getOperand(0), OS);
  }
  printAnnotation(OS, Annot);
}
