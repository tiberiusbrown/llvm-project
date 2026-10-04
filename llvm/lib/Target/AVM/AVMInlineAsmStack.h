//===-- AVMInlineAsmStack.h - Inline assembly stack checks ------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMINLINEASMSTACK_H
#define LLVM_LIB_TARGET_AVM_AVMINLINEASMSTACK_H

#include "llvm/ADT/StringRef.h"

namespace llvm {
class MCContext;
class MCSubtargetInfo;
class TargetMachine;

// Inspect substituted assembly in an isolated MC context. This proves only
// stack depth and control-flow containment, not pointer or memory effects.
bool isAVMInlineAsmStackSafe(StringRef Assembly, const TargetMachine &TM,
                             const MCSubtargetInfo &STI, MCContext &Original);
} // namespace llvm

#endif
