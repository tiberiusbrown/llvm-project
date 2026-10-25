//===-- AVMSubtarget.cpp - AVM subtarget information ---------------------===//

#include "AVMSubtarget.h"
#include "AVMSelectionDAGInfo.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGISel.h"

using namespace llvm;

#define DEBUG_TYPE "avm-subtarget"
#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AVMGenSubtargetInfo.inc"

namespace {
// The interpreter cannot overlap independent operations. In a large scalar
// reduction, scheduling those operations ahead of their consumers only keeps
// more partial results live. Use register-reduction scheduling when the number
// of distinct inputs exceeds the register file; retain the existing scheduler
// for other DAGs, where its copy/coalescing choices can still be profitable.
static bool hasLargeReduction(const SelectionDAG &DAG) {
  for (const SDNode &Root : DAG.allnodes()) {
    if (!Root.isMachineOpcode())
      continue;
    unsigned Opcode = Root.getMachineOpcode();
    switch (Opcode) {
    case AVM::ADD16_PSEUDO:
    case AVM::AND16_PSEUDO:
    case AVM::OR16_PSEUDO:
    case AVM::XOR16_PSEUDO:
      break;
    default:
      continue;
    }
    // Visit each single-use reduction tree once, starting at its root. Shared
    // subexpressions remain inputs and are considered separately.
    if (Root.hasOneUse()) {
      const SDNode *User = Root.use_begin()->getUser();
      if (User->isMachineOpcode() && User->getMachineOpcode() == Opcode)
        continue;
    }

    SmallVector<const SDNode *, 16> Worklist{&Root};
    SmallPtrSet<const SDNode *, 16> Inputs;
    while (!Worklist.empty()) {
      const SDNode *N = Worklist.pop_back_val();
      if (N->isMachineOpcode() && N->getMachineOpcode() == Opcode &&
          (N == &Root || N->hasOneUse())) {
        Worklist.push_back(N->getOperand(0).getNode());
        Worklist.push_back(N->getOperand(1).getNode());
      } else if (!isa<ConstantSDNode>(N)) {
        Inputs.insert(N);
        if (Inputs.size() > AVM::GPR16RegClass.getNumRegs())
          return true;
      }
    }
  }
  return false;
}

static ScheduleDAGSDNodes *createAVMDAGScheduler(SelectionDAGISel *IS,
                                               CodeGenOptLevel OptLevel) {
  if (hasLargeReduction(*IS->CurDAG))
    return createBURRListDAGScheduler(IS, OptLevel);
  return createILPListDAGScheduler(IS, OptLevel);
}
} // namespace

RegisterScheduler::FunctionPassCtor
AVMSubtarget::getDAGScheduler(CodeGenOptLevel OptLevel) const {
  return OptLevel == CodeGenOptLevel::None ? nullptr : createAVMDAGScheduler;
}

AVMSubtarget &AVMSubtarget::initializeSubtargetDependencies(StringRef CPU,
                                                            StringRef TuneCPU,
                                                            StringRef FS) {
  if (CPU.empty())
    CPU = "avm1";
  ParseSubtargetFeatures(CPU, TuneCPU, FS);
  return *this;
}

AVMSubtarget::AVMSubtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU,
                           StringRef FS, const TargetMachine &TM)
    : AVMGenSubtargetInfo(TT, CPU, TuneCPU, FS),
      InstrInfo(initializeSubtargetDependencies(CPU, TuneCPU, FS)),
      TLInfo(TM, *this), FrameLowering(),
      TSInfo(std::make_unique<AVMSelectionDAGInfo>()) {}

AVMSubtarget::~AVMSubtarget() = default;
