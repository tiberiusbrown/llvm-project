//===-- AVMMCInstLower.h - Lower AVM MachineInstr to MCInst ----*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMMCINSTLOWER_H
#define LLVM_LIB_TARGET_AVM_AVMMCINSTLOWER_H

namespace llvm {
class AsmPrinter;
class MCContext;
class MCInst;
class MachineInstr;

class AVMMCInstLower final {
  MCContext &Ctx;
  AsmPrinter &Printer;

public:
  AVMMCInstLower(MCContext &Ctx, AsmPrinter &Printer)
      : Ctx(Ctx), Printer(Printer) {}

  void lower(const MachineInstr *MI, MCInst &OutMI) const;
};
} // namespace llvm

#endif
