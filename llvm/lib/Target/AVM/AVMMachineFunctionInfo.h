//===-- AVMMachineFunctionInfo.h - AVM machine function info -*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_AVM_AVMMACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Metadata.h"

namespace llvm {
class AVMMachineFunctionInfo final : public MachineFunctionInfo {
  void anchor();

  Register SRetReturnReg;
  int VarArgsFrameIndex = 0;
  SmallVector<const MDNode *, 0> StackTargets;

public:
  AVMMachineFunctionInfo() = default;
  AVMMachineFunctionInfo(const Function &, const TargetSubtargetInfo *) {}

  Register getSRetReturnReg() const { return SRetReturnReg; }
  void setSRetReturnReg(Register Reg) { SRetReturnReg = Reg; }
  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int Index) { VarArgsFrameIndex = Index; }

  // Zero means unknown. IDs survive machine block duplication and tail-call
  // rewrites without adding operands to the actual machine instructions.
  unsigned addStackTargets(const CallBase *CB) {
    const MDNode *Targets =
        CB ? CB->getMetadata(LLVMContext::MD_callees) : nullptr;
    if (!Targets)
      return 0;
    StackTargets.push_back(Targets);
    return StackTargets.size();
  }
  const MDNode *getStackTargets(unsigned ID) const {
    return ID ? StackTargets[ID - 1] : nullptr;
  }

  MachineFunctionInfo *
  clone(BumpPtrAllocator &Allocator, MachineFunction &DestMF,
        const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
      const override;
};
} // namespace llvm

#endif
