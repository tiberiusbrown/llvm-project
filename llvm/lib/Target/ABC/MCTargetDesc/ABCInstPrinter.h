//===-- ABCInstPrinter.h - Convert ABC MCInst to assembly ------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCINSTPRINTER_H
#define LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCINSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"

namespace llvm {
class ABCInstPrinter : public MCInstPrinter {
public:
  ABCInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                 const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

  void printRegName(raw_ostream &O, MCRegister Reg) override;
  void printInst(const MCInst *MI, uint64_t Address, StringRef Annot,
                 const MCSubtargetInfo &STI, raw_ostream &O) override;
  std::pair<const char *, uint64_t> getMnemonic(const MCInst &MI) const override;
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCINSTPRINTER_H
