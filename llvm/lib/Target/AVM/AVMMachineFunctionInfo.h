//===-- AVMMachineFunctionInfo.h - AVM machine function info -*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_AVM_AVMMACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {
class AVMMachineFunctionInfo final : public MachineFunctionInfo {
  void anchor();

  Register SRetReturnReg;
  int VarArgsFrameIndex = 0;

public:
  AVMMachineFunctionInfo() = default;
  AVMMachineFunctionInfo(const Function &, const TargetSubtargetInfo *) {}

  Register getSRetReturnReg() const { return SRetReturnReg; }
  void setSRetReturnReg(Register Reg) { SRetReturnReg = Reg; }
  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int Index) { VarArgsFrameIndex = Index; }

  MachineFunctionInfo *
  clone(BumpPtrAllocator &Allocator, MachineFunction &DestMF,
        const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
      const override;
};
} // namespace llvm

#endif
