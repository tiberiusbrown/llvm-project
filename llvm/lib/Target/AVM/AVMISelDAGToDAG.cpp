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

  bool matchFrameAddress(SDValue Address, int &FrameIndex,
                         int64_t &Offset) const {
    if (const auto *FI = dyn_cast<FrameIndexSDNode>(Address)) {
      FrameIndex = FI->getIndex();
      Offset = 0;
      return true;
    }
    if (Address.getOpcode() != ISD::ADD)
      return false;
    const auto *FI = dyn_cast<FrameIndexSDNode>(Address.getOperand(0));
    const auto *Imm = dyn_cast<ConstantSDNode>(Address.getOperand(1));
    if (!FI || !Imm)
      return false;
    FrameIndex = FI->getIndex();
    Offset = Imm->getSExtValue();
    return true;
  }

  bool selectCall(SDNode *Node) {
    SDValue Callee = Node->getOperand(1);
    bool IsDirect = Callee.getOpcode() == ISD::TargetGlobalAddress ||
                    Callee.getOpcode() == ISD::TargetExternalSymbol;
    unsigned Opcode =
        IsDirect ? AVM::CALL_DIRECT_PSEUDO : AVM::CALL_INDIRECT_PSEUDO;

    unsigned Last = Node->getNumOperands() - 1;
    bool HasGlue = Node->getOperand(Last).getValueType() == MVT::Glue;
    if (HasGlue)
      --Last;

    SmallVector<SDValue, 10> Ops;
    Ops.push_back(Callee);
    for (unsigned I = 2; I <= Last; ++I)
      Ops.push_back(Node->getOperand(I));
    Ops.push_back(Node->getOperand(0));
    if (HasGlue)
      Ops.push_back(Node->getOperand(Node->getNumOperands() - 1));

    SDNode *Call =
        CurDAG->getMachineNode(Opcode, SDLoc(Node), MVT::Other, MVT::Glue, Ops);
    ReplaceUses(SDValue(Node, 0), SDValue(Call, 0));
    ReplaceUses(SDValue(Node, 1), SDValue(Call, 1));
    CurDAG->RemoveDeadNode(Node);
    return true;
  }

  bool selectLoad24(SDNode *Node) {
    const auto *FI = dyn_cast<FrameIndexSDNode>(Node->getOperand(1));
    if (!FI)
      return false;
    SDValue Ops[] = {CurDAG->getTargetFrameIndex(FI->getIndex(), MVT::i16),
                     CurDAG->getTargetConstant(0, SDLoc(Node), MVT::i16),
                     Node->getOperand(0)};
    SDNode *Result = CurDAG->getMachineNode(
        AVM::STACK_LOAD24_PSEUDO, SDLoc(Node), MVT::i32, MVT::Other, Ops);
    ReplaceUses(SDValue(Node, 0), SDValue(Result, 0));
    ReplaceUses(SDValue(Node, 1), SDValue(Result, 1));
    CurDAG->RemoveDeadNode(Node);
    return true;
  }

  bool selectStore24(SDNode *Node) {
    SDValue Ops[] = {
        CurDAG->getTargetConstant(
            cast<ConstantSDNode>(Node->getOperand(2))->getZExtValue(),
            SDLoc(Node), MVT::i16),
        Node->getOperand(1), Node->getOperand(0)};
    SDNode *Result = CurDAG->getMachineNode(AVM::OUT_STORE24_PSEUDO,
                                            SDLoc(Node), MVT::Other, Ops);
    ReplaceUses(SDValue(Node, 0), SDValue(Result, 0));
    CurDAG->RemoveDeadNode(Node);
    return true;
  }

  bool selectStackLoad(SDNode *Node) {
    const auto *Load = cast<LoadSDNode>(Node);
    int FrameIndex;
    int64_t Offset;
    if (!matchFrameAddress(Load->getBasePtr(), FrameIndex, Offset) ||
        Load->getAddressingMode() != ISD::UNINDEXED)
      return false;

    unsigned Opcode;
    if (Load->getMemoryVT() == MVT::i8 && Load->getValueType(0) == MVT::i16) {
      Opcode = Load->getExtensionType() == ISD::SEXTLOAD
                   ? AVM::STACK_LOAD8S_PSEUDO
                   : AVM::STACK_LOAD8U_PSEUDO;
    } else if (Load->getMemoryVT() == MVT::i16 &&
               Load->getValueType(0) == MVT::i16) {
      Opcode = AVM::STACK_LOAD16_PSEUDO;
    } else if (Load->getMemoryVT() == MVT::i32 &&
               Load->getValueType(0) == MVT::i32) {
      Opcode = AVM::STACK_LOAD32_PSEUDO;
    } else {
      return false;
    }

    SDValue TFI = CurDAG->getTargetFrameIndex(FrameIndex, MVT::i16);
    SDValue Ops[] = {TFI,
                     CurDAG->getTargetConstant(Offset, SDLoc(Node), MVT::i16),
                     Load->getChain()};
    SDNode *Result = CurDAG->getMachineNode(
        Opcode, SDLoc(Node), Load->getValueType(0), MVT::Other, Ops);
    CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                           {Load->getMemOperand()});
    ReplaceUses(SDValue(Node, 0), SDValue(Result, 0));
    ReplaceUses(SDValue(Node, 1), SDValue(Result, 1));
    CurDAG->RemoveDeadNode(Node);
    return true;
  }

  bool selectStackStore(SDNode *Node) {
    const auto *Store = cast<StoreSDNode>(Node);
    int FrameIndex;
    int64_t Offset;
    if (!matchFrameAddress(Store->getBasePtr(), FrameIndex, Offset) ||
        Store->getAddressingMode() != ISD::UNINDEXED)
      return false;

    unsigned Opcode;
    if (Store->getMemoryVT() == MVT::i8)
      Opcode = AVM::STACK_STORE8_PSEUDO;
    else if (Store->getMemoryVT() == MVT::i16)
      Opcode = AVM::STACK_STORE16_PSEUDO;
    else if (Store->getMemoryVT() == MVT::i32)
      Opcode = AVM::STACK_STORE32_PSEUDO;
    else
      return false;

    SDValue Ops[] = {CurDAG->getTargetFrameIndex(FrameIndex, MVT::i16),
                     CurDAG->getTargetConstant(Offset, SDLoc(Node), MVT::i16),
                     Store->getValue(), Store->getChain()};
    SDNode *Result =
        CurDAG->getMachineNode(Opcode, SDLoc(Node), MVT::Other, Ops);
    CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                           {Store->getMemOperand()});
    ReplaceUses(SDValue(Node, 0), SDValue(Result, 0));
    CurDAG->RemoveDeadNode(Node);
    return true;
  }

  bool selectOutgoingStore(SDNode *Node) {
    const auto *Store = cast<StoreSDNode>(Node);
    SDValue Base = Store->getBasePtr();
    int64_t Offset = 0;

    if (const auto *Reg = dyn_cast<RegisterSDNode>(Base)) {
      if (Reg->getReg() != AVM::SP)
        return false;
    } else if (Base.getOpcode() == ISD::ADD) {
      const auto *Reg = dyn_cast<RegisterSDNode>(Base.getOperand(0));
      const auto *Imm = dyn_cast<ConstantSDNode>(Base.getOperand(1));
      if (!Reg || Reg->getReg() != AVM::SP || !Imm)
        return false;
      Offset = Imm->getSExtValue();
    } else {
      return false;
    }

    if (!isUInt<8>(Offset))
      report_fatal_error("AVM outgoing stack argument offset exceeds u8");
    unsigned Opcode;
    if (Store->getMemoryVT() == MVT::i8)
      Opcode = AVM::OUT_STORE8_PSEUDO;
    else if (Store->getMemoryVT() == MVT::i16)
      Opcode = AVM::OUT_STORE16_PSEUDO;
    else if (Store->getMemoryVT() == MVT::i32)
      Opcode = AVM::OUT_STORE32_PSEUDO;
    else
      return false;

    SDValue Ops[] = {CurDAG->getTargetConstant(Offset, SDLoc(Node), MVT::i16),
                     Store->getValue(), Store->getChain()};
    SDNode *Result =
        CurDAG->getMachineNode(Opcode, SDLoc(Node), MVT::Other, Ops);
    CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                           {Store->getMemOperand()});
    ReplaceUses(SDValue(Node, 0), SDValue(Result, 0));
    CurDAG->RemoveDeadNode(Node);
    return true;
  }

  bool selectFrameIndex(SDNode *Node) {
    const auto *FI = cast<FrameIndexSDNode>(Node);
    SDValue TFI =
        CurDAG->getTargetFrameIndex(FI->getIndex(), Node->getValueType(0));
    CurDAG->SelectNodeTo(Node, AVM::FRAMEADDR_PSEUDO, Node->getValueType(0),
                         TFI,
                         CurDAG->getTargetConstant(0, SDLoc(Node), MVT::i16));
    return true;
  }

  bool selectFrameAddress(SDNode *Node) {
    int FrameIndex;
    int64_t Offset;
    if (!matchFrameAddress(SDValue(Node, 0), FrameIndex, Offset))
      return false;
    SDValue TFI = CurDAG->getTargetFrameIndex(FrameIndex, MVT::i16);
    CurDAG->SelectNodeTo(
        Node, AVM::FRAMEADDR_PSEUDO, Node->getValueType(0), TFI,
        CurDAG->getTargetConstant(Offset, SDLoc(Node), MVT::i16));
    return true;
  }

  void Select(SDNode *Node) override {
    if (Node->isMachineOpcode()) {
      Node->setNodeId(-1);
      return;
    }
    switch (Node->getOpcode()) {
    case AVMISD::CALL:
      selectCall(Node);
      return;
    case AVMISD::LOAD24:
      if (selectLoad24(Node))
        return;
      break;
    case AVMISD::STORE24:
      selectStore24(Node);
      return;
    case ISD::LOAD:
      if (selectStackLoad(Node))
        return;
      break;
    case ISD::STORE:
      if (selectStackStore(Node) || selectOutgoingStore(Node))
        return;
      break;
    case ISD::ADD:
      if (selectFrameAddress(Node))
        return;
      break;
    case ISD::FrameIndex:
      selectFrameIndex(Node);
      return;
    default:
      break;
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
