//===-- AVMSelectionDAGInfo.cpp - AVM SelectionDAG information -----------===//

#include "AVMSelectionDAGInfo.h"
#include "AVM.h"
#include "AVMISelLowering.h"
#include "AVMInstrInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/SelectionDAG.h"

using namespace llvm;

namespace {

static LocationSize getServiceMemorySize(SDValue Size) {
  if (const auto *C = dyn_cast<ConstantSDNode>(Size))
    return LocationSize::precise(C->getZExtValue());
  return LocationSize::afterPointer();
}

static SDValue stripProgramPointerNormalization(SDValue Value) {
  for (;;) {
    if (Value.getOpcode() == AVMISD::NORMALIZE_PROGPTR) {
      Value = Value.getOperand(0);
      continue;
    }
    if (Value.getOpcode() == ISD::ZERO_EXTEND &&
        Value.getOperand(0).getOpcode() == ISD::TRUNCATE &&
        Value.getOperand(0).getValueType() == MVT::i24) {
      Value = Value.getOperand(0).getOperand(0);
      continue;
    }
    if (Value.getOpcode() != ISD::AND || Value.getValueType() != MVT::i32)
      return Value;
    SDValue Candidate = Value.getOperand(0);
    const auto *Mask = dyn_cast<ConstantSDNode>(Value.getOperand(1));
    if (!Mask) {
      Mask = dyn_cast<ConstantSDNode>(Candidate);
      Candidate = Value.getOperand(1);
    }
    if (!Mask || Mask->getZExtValue() != 0xffffff)
      return Value;
    Value = Candidate;
  }
}

static SDValue
emitCopyOrMoveService(unsigned Opcode, SelectionDAG &DAG, const SDLoc &DL,
                      SDValue Chain, SDValue Dst, SDValue Src, SDValue Size,
                      Align Alignment, MachinePointerInfo DstPtrInfo,
                      MachinePointerInfo SrcPtrInfo, const AAMDNodes &AAInfo) {
  MachineFunction &MF = DAG.getMachineFunction();
  LocationSize MemSize = getServiceMemorySize(Size);
  MachineMemOperand *DstMMO = MF.getMachineMemOperand(
      DstPtrInfo, MachineMemOperand::MOStore, MemSize, Alignment, AAInfo);
  MachineMemOperand *SrcMMO = MF.getMachineMemOperand(
      SrcPtrInfo, MachineMemOperand::MOLoad, MemSize, Alignment, AAInfo);

  SmallVector<SDValue, 4> Ops;
  Ops.push_back(Dst);
  if (Opcode == AVM::SYS_MEMCPY_P_PSEUDO) {
    // Physical service order is dst, size, src.
    Ops.push_back(Size);
    Ops.push_back(stripProgramPointerNormalization(Src));
  } else {
    Ops.push_back(Src);
    Ops.push_back(Size);
  }
  Ops.push_back(Chain);

  EVT ResultVTs[] = {MVT::i16, MVT::Other};
  MachineSDNode *Node = DAG.getMachineNode(Opcode, DL, ResultVTs, Ops);
  DAG.setNodeMemRefs(Node, {DstMMO, SrcMMO});
  return SDValue(Node, 1);
}

} // namespace

SDValue AVMSelectionDAGInfo::EmitTargetCodeForMemcpyWithAA(
    SelectionDAG &DAG, const SDLoc &DL, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool IsVolatile, bool AlwaysInline,
    MachinePointerInfo DstPtrInfo, MachinePointerInfo SrcPtrInfo,
    const AAMDNodes &AAInfo) const {
  if (IsVolatile || AlwaysInline || Size.getValueType() != MVT::i16 ||
      DstPtrInfo.getAddrSpace() != 0)
    return {};

  unsigned Opcode;
  if (SrcPtrInfo.getAddrSpace() == 0)
    Opcode = AVM::SYS_MEMCPY_PSEUDO;
  else if (SrcPtrInfo.getAddrSpace() == 1)
    Opcode = AVM::SYS_MEMCPY_P_PSEUDO;
  else
    return {};

  return emitCopyOrMoveService(Opcode, DAG, DL, Chain, Dst, Src, Size,
                               Alignment, DstPtrInfo, SrcPtrInfo, AAInfo);
}

SDValue AVMSelectionDAGInfo::EmitTargetCodeForMemmoveWithAA(
    SelectionDAG &DAG, const SDLoc &DL, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool IsVolatile,
    MachinePointerInfo DstPtrInfo, MachinePointerInfo SrcPtrInfo,
    const AAMDNodes &AAInfo) const {
  if (IsVolatile || Size.getValueType() != MVT::i16 ||
      DstPtrInfo.getAddrSpace() != 0 || SrcPtrInfo.getAddrSpace() != 0)
    return {};
  return emitCopyOrMoveService(AVM::SYS_MEMMOVE_PSEUDO, DAG, DL, Chain, Dst,
                               Src, Size, Alignment, DstPtrInfo, SrcPtrInfo,
                               AAInfo);
}

SDValue AVMSelectionDAGInfo::EmitTargetCodeForMemsetWithAA(
    SelectionDAG &DAG, const SDLoc &DL, SDValue Chain, SDValue Dst,
    SDValue Value, SDValue Size, Align Alignment, bool IsVolatile,
    bool AlwaysInline, MachinePointerInfo DstPtrInfo,
    const AAMDNodes &AAInfo) const {
  if (IsVolatile || AlwaysInline || Size.getValueType() != MVT::i16 ||
      DstPtrInfo.getAddrSpace() != 0)
    return {};

  if (Value.getValueType() != MVT::i16)
    Value = DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i16, Value);

  MachineFunction &MF = DAG.getMachineFunction();
  MachineMemOperand *DstMMO =
      MF.getMachineMemOperand(DstPtrInfo, MachineMemOperand::MOStore,
                              getServiceMemorySize(Size), Alignment, AAInfo);
  SDValue Ops[] = {Dst, Value, Size, Chain};
  EVT ResultVTs[] = {MVT::i16, MVT::Other};
  MachineSDNode *Node =
      DAG.getMachineNode(AVM::SYS_MEMSET_PSEUDO, DL, ResultVTs, Ops);
  DAG.setNodeMemRefs(Node, {DstMMO});
  return SDValue(Node, 1);
}
