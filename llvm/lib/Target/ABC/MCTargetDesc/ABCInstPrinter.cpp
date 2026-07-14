//===-- ABCInstPrinter.cpp - Convert ABC MCInst to assembly ---------------===//

#include "ABCInstrFormats.h"
#include "ABCInstPrinter.h"
#include "ABCMCTargetDesc.h"
#include "../ABCSyscalls.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

void ABCInstPrinter::printRegName(raw_ostream &O, MCRegister Reg) {
  O << "stk";
}

static void printOperand(const MCAsmInfo &MAI, const MCOperand &Op,
                         raw_ostream &O) {
  if (Op.isImm())
    O << Op.getImm();
  else if (Op.isExpr())
    MAI.printExpr(O, *Op.getExpr());
  else if (Op.isReg())
    O << "stk";
}

static void printSyscallOperand(const MCAsmInfo &MAI, const MCOperand &Op,
                                raw_ostream &O) {
  if (!Op.isImm()) {
    printOperand(MAI, Op, O);
    return;
  }

  StringRef Name = getABCSyscallName(Op.getImm());
  if (!Name.empty())
    O << Name;
  else
    O << Op.getImm();
}

void ABCInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                               StringRef Annot, const MCSubtargetInfo &STI,
                               raw_ostream &O) {
  const ABC::InstrDesc *Desc = ABC::getInstrDescByOpcode(MI->getOpcode());
  if (!Desc) {
    O << "<unknown>";
    printAnnotation(O, Annot);
    return;
  }
  O << Desc->Mnemonic;
  for (unsigned I = 0; I < Desc->NumOperands; ++I) {
    O << (I == 0 ? "\t" : ", ");
    if (MI->getOpcode() == ABC::SYS)
      printSyscallOperand(MAI, MI->getOperand(I), O);
    else
      printOperand(MAI, MI->getOperand(I), O);
  }
  printAnnotation(O, Annot);
}

std::pair<const char *, uint64_t>
ABCInstPrinter::getMnemonic(const MCInst &MI) const {
  if (const ABC::InstrDesc *Desc = ABC::getInstrDescByOpcode(MI.getOpcode()))
    return {Desc->Mnemonic, 0};
  return {"", 0};
}
