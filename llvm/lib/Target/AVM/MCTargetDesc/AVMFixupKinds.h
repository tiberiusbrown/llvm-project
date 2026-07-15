#ifndef LLVM_LIB_TARGET_AVM_MCTARGETDESC_AVMFIXUPKINDS_H
#define LLVM_LIB_TARGET_AVM_MCTARGETDESC_AVMFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm::AVM {

enum Fixups : unsigned {
  fixup_avm_data16 = FirstTargetFixupKind,
  fixup_avm_prog24,
  fixup_avm_prog_lo16,
  fixup_avm_prog_hi8,
  fixup_avm_pcrel8,
  fixup_avm_pcrel16,
  fixup_avm_far24,
  fixup_avm_relax,
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};

} // namespace llvm::AVM

#endif
