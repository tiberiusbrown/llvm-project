//===-- AVMMCExpr.cpp - AVM-specific MC expressions ---------------------===//

#include "AVMMCExpr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCValue.h"

using namespace llvm;

const AVMMCExpr *AVMMCExpr::create(VariantKind Kind, const MCExpr *Expr,
                                   MCContext &Ctx) {
  return new (Ctx) AVMMCExpr(Kind, Expr);
}

StringRef AVMMCExpr::getVariantName() const {
  return Kind == VK_ProgHi8 ? "prog_hi8" : "prog_lo16";
}

void AVMMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  OS << getVariantName() << '(';
  MAI->printExpr(OS, *Expr);
  OS << ')';
}

bool AVMMCExpr::evaluateAsRelocatableImpl(MCValue &Res,
                                          const MCAssembler *Asm) const {
  return Expr->evaluateAsRelocatable(Res, Asm);
}

void AVMMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*Expr);
}

MCFragment *AVMMCExpr::findAssociatedFragment() const {
  return Expr->findAssociatedFragment();
}
