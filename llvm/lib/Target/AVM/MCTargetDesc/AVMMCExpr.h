#ifndef LLVM_LIB_TARGET_AVM_MCTARGETDESC_AVMMCEXPR_H
#define LLVM_LIB_TARGET_AVM_MCTARGETDESC_AVMMCEXPR_H

#include "llvm/MC/MCExpr.h"

namespace llvm::AVM {
enum ExprKind : uint16_t {
  VK_AVM_LO16 = MCSymbolRefExpr::FirstTargetSpecifier,
  VK_AVM_HI8,
  VK_AVM_PROG24,
};

inline bool isAVMExprKind(uint32_t Kind) {
  return Kind >= VK_AVM_LO16 && Kind <= VK_AVM_PROG24;
}
} // namespace llvm::AVM

#endif
