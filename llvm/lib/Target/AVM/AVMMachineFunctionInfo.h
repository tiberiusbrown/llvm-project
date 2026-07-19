//===-- AVMMachineFunctionInfo.h - AVM machine function info -*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_AVM_AVMMACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {
class AVMMachineFunctionInfo final : public MachineFunctionInfo {
  void anchor();

public:
  AVMMachineFunctionInfo() = default;
  AVMMachineFunctionInfo(const Function &, const TargetSubtargetInfo *) {}

  MachineFunctionInfo *
  clone(BumpPtrAllocator &Allocator, MachineFunction &DestMF,
        const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
      const override;
};
} // namespace llvm

#endif
