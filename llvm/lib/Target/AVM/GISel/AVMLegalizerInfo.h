#ifndef LLVM_LIB_TARGET_AVM_GISEL_AVMLEGALIZERINFO_H
#define LLVM_LIB_TARGET_AVM_GISEL_AVMLEGALIZERINFO_H

#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"

namespace llvm {
class AVMSubtarget;

class AVMLegalizerInfo final : public LegalizerInfo {
public:
  explicit AVMLegalizerInfo(const AVMSubtarget &STI);
};
} // namespace llvm

#endif
