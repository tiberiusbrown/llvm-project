//===-- AVMMCExpr.h - AVM-specific MC expressions ------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_MCTARGETDESC_AVMMCEXPR_H
#define LLVM_LIB_TARGET_AVM_MCTARGETDESC_AVMMCEXPR_H

#include "llvm/MC/MCExpr.h"

namespace llvm {

class AVMMCExpr final : public MCTargetExpr {
public:
  enum VariantKind { VK_ProgLo16, VK_ProgHi8 };

  static const AVMMCExpr *create(VariantKind Kind, const MCExpr *Expr,
                                 MCContext &Ctx);

  VariantKind getVariantKind() const { return Kind; }
  StringRef getVariantName() const;
  const MCExpr *getSubExpr() const { return Expr; }

  void printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const override;
  bool evaluateAsRelocatableImpl(MCValue &Res,
                                 const MCAssembler *Asm) const override;
  void visitUsedExpr(MCStreamer &Streamer) const override;
  MCFragment *findAssociatedFragment() const override;

  static bool classof(const MCExpr *Expr) {
    return Expr->getKind() == MCExpr::Target;
  }

private:
  AVMMCExpr(VariantKind Kind, const MCExpr *Expr) : Kind(Kind), Expr(Expr) {}

  VariantKind Kind;
  const MCExpr *Expr;
};

} // namespace llvm

#endif
