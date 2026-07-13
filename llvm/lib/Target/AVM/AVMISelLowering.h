#ifndef LLVM_LIB_TARGET_AVM_AVMISELLOWERING_H
#define LLVM_LIB_TARGET_AVM_AVMISELLOWERING_H

#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class AVMSubtarget;

class AVMTargetLowering final : public TargetLowering {
public:
  AVMTargetLowering(const TargetMachine &TM, const AVMSubtarget &STI);

  bool isSelectSupported(SelectSupportKind) const override { return false; }
};
} // namespace llvm

#endif
