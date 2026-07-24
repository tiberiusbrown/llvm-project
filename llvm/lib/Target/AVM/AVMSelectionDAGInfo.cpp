//===-- AVMSelectionDAGInfo.cpp - AVM SelectionDAG information -----------===//

#include "AVMSelectionDAGInfo.h"
#include "AVM.h"
#include "AVMISelLowering.h"
#include "AVMInstrInfo.h"
#include "AVMSystemServiceInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/SelectionDAG.h"

using namespace llvm;

namespace {

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
  const AVMSystemServiceInfo &Info = getRequiredAVMSystemServiceInfo(Opcode);
  MachineFunction &MF = DAG.getMachineFunction();
  SDValue LogicalOps[] = {Dst, Src, Size};

  SmallVector<SDValue, 4> Ops;
  for (const AVMServiceInputInfo &Input : Info.Inputs) {
    assert(Input.LogicalArgumentIndex < std::size(LogicalOps) &&
           "memory intrinsic and service descriptor disagree");
    SDValue Value = LogicalOps[Input.LogicalArgumentIndex];
    if (Input.PointerPolicy == AVMServicePointerPolicy::IgnorePadding)
      Value = stripProgramPointerNormalization(Value);
    assert(Input.PointerPolicy != AVMServicePointerPolicy::RequireNormalized &&
           "generic memory service unexpectedly requires normalization");
    if (Input.PassToMachine)
      Ops.push_back(Value);
  }
  Ops.push_back(Chain);

  SmallVector<EVT, 2> ResultVTs;
  for (const AVMServiceOutputInfo &Output : Info.Outputs) {
    switch (Output.Kind) {
    case AVMServiceValueKind::I16:
      ResultVTs.push_back(MVT::i16);
      break;
    case AVMServiceValueKind::I32:
    case AVMServiceValueKind::ProgramPointer:
      ResultVTs.push_back(MVT::i32);
      break;
    case AVMServiceValueKind::F32:
      ResultVTs.push_back(MVT::f32);
      break;
    }
  }
  ResultVTs.push_back(MVT::Other);
  MachineSDNode *Node = DAG.getMachineNode(Opcode, DL, ResultVTs, Ops);

  SmallVector<MachineMemOperand *, 2> MMOs;
  for (const AVMServiceMemoryAccessInfo &Access : Info.MemoryAccesses) {
    assert(Access.BaseKind == AVMServiceMemoryBaseKind::LogicalArgument &&
           "generic memory service needs a logical memory base");
    MachinePointerInfo PtrInfo =
        Access.LogicalArgumentIndex == 0 ? DstPtrInfo : SrcPtrInfo;
    LocationSize MemSize = LocationSize::afterPointer();
    if (Access.SizeKind == AVMServiceMemorySizeKind::Constant)
      MemSize = LocationSize::precise(Access.ConstantSize);
    else if (Access.SizeKind == AVMServiceMemorySizeKind::LogicalArgument)
      if (const auto *C = dyn_cast<ConstantSDNode>(
              LogicalOps[Access.SizeLogicalArgumentIndex]))
        MemSize = LocationSize::precise(C->getZExtValue());
    MMOs.push_back(MF.getMachineMemOperand(PtrInfo, Access.Flags, MemSize,
                                           Alignment, AAInfo));
  }
  DAG.setNodeMemRefs(Node, MMOs);
  return SDValue(Node, Info.Outputs.size());
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

  const AVMSystemServiceInfo &Info =
      getRequiredAVMSystemServiceInfo(AVM::SYS_MEMSET_PSEUDO);
  MachineFunction &MF = DAG.getMachineFunction();
  SDValue LogicalOps[] = {Dst, Value, Size};
  SmallVector<SDValue, 4> Ops;
  for (const AVMServiceInputInfo &Input : Info.Inputs)
    if (Input.PassToMachine)
      Ops.push_back(LogicalOps[Input.LogicalArgumentIndex]);
  Ops.push_back(Chain);
  SmallVector<EVT, 2> ResultVTs = {MVT::i16, MVT::Other};
  MachineSDNode *Node =
      DAG.getMachineNode(AVM::SYS_MEMSET_PSEUDO, DL, ResultVTs, Ops);
  SmallVector<MachineMemOperand *, 1> MMOs;
  for (const AVMServiceMemoryAccessInfo &Access : Info.MemoryAccesses) {
    LocationSize MemSize = LocationSize::afterPointer();
    if (const auto *C = dyn_cast<ConstantSDNode>(
            LogicalOps[Access.SizeLogicalArgumentIndex]))
      MemSize = LocationSize::precise(C->getZExtValue());
    MMOs.push_back(MF.getMachineMemOperand(DstPtrInfo, Access.Flags, MemSize,
                                           Alignment, AAInfo));
  }
  DAG.setNodeMemRefs(Node, MMOs);
  return SDValue(Node, Info.Outputs.size());
}
