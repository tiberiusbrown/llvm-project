//===-- AVMMachineFunctionInfo.cpp - AVM machine function info -----------===//

#include "AVMMachineFunctionInfo.h"

using namespace llvm;

void AVMMachineFunctionInfo::anchor() {}

MachineFunctionInfo *AVMMachineFunctionInfo::clone(
    BumpPtrAllocator &, MachineFunction &DestMF,
    const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &) const {
  return DestMF.cloneInfo<AVMMachineFunctionInfo>(*this);
}
