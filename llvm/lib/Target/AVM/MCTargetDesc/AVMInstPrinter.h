#ifndef LLVM_LIB_TARGET_AVM_MCTARGETDESC_AVMINSTPRINTER_H
#define LLVM_LIB_TARGET_AVM_MCTARGETDESC_AVMINSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"

namespace llvm {

class AVMInstPrinter final : public MCInstPrinter {
public:
  AVMInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                 const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

  std::pair<const char *, uint64_t>
  getMnemonic(const MCInst &MI) const override;
  void printInst(const MCInst *MI, uint64_t Address, StringRef Annot,
                 const MCSubtargetInfo &STI, raw_ostream &OS) override;
  void printRegName(raw_ostream &OS, MCRegister Reg) override;

private:
  void printOperand(const MCOperand &Op, raw_ostream &OS) const;
  void printCompactReg(MCRegister Reg, raw_ostream &OS) const;
  void printFullReg(MCRegister Reg, raw_ostream &OS) const;
};

} // namespace llvm

#endif
