//===-- ABCFixupKinds.h - ABC-specific fixup entries ----------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCFIXUPKINDS_H
#define LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace ABC {
enum Fixups {
  fixup_abc_8 = FirstTargetFixupKind,
  fixup_abc_16,
  fixup_abc_24,
  fixup_abc_32,
  fixup_abc_prog24,
  fixup_abc_global16_tagged,
  fixup_abc_global8,
  fixup_abc_branch8,
  fixup_abc_branch16,
  fixup_abc_call24,
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
} // namespace ABC
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCFIXUPKINDS_H
