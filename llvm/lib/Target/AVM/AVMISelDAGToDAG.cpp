//===-- AVMISelDAGToDAG.cpp - AVM DAG instruction selector ---------------===//

#include "AVM.h"
#include "AVMTargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"

using namespace llvm;

#define DEBUG_TYPE "avm-isel"
#define PASS_NAME "AVM DAG->DAG Pattern Instruction Selection"

namespace {
class AVMDAGToDAGISel final : public SelectionDAGISel {
public:
  AVMDAGToDAGISel(AVMTargetMachine &TM, CodeGenOptLevel OptLevel)
      : SelectionDAGISel(TM, OptLevel) {}

private:
#include "AVMGenDAGISel.inc"

  void Select(SDNode *Node) override {
    if (Node->isMachineOpcode()) {
      Node->setNodeId(-1);
      return;
    }
    SelectCode(Node);
  }
};

class AVMDAGToDAGISelLegacy final : public SelectionDAGISelLegacy {
public:
  static char ID;
  AVMDAGToDAGISelLegacy(AVMTargetMachine &TM, CodeGenOptLevel OptLevel)
      : SelectionDAGISelLegacy(
            ID, std::make_unique<AVMDAGToDAGISel>(TM, OptLevel)) {}
};
} // namespace

char AVMDAGToDAGISelLegacy::ID = 0;

INITIALIZE_PASS(AVMDAGToDAGISelLegacy, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVMISelDag(AVMTargetMachine &TM,
                                     CodeGenOptLevel OptLevel) {
  return new AVMDAGToDAGISelLegacy(TM, OptLevel);
}
