//===-- AVMISelLowering.h - AVM DAG lowering interface --------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMISELLOWERING_H
#define LLVM_LIB_TARGET_AVM_AVMISELLOWERING_H

#include "AVM.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class AVMSubtarget;

namespace AVMISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  BR_CC,
  CALL,
  CMOV,
  CMP,
  CMPI,
  CSET,
  LOAD24,
  STORE24,
  TST8,
  TST16,
  WRAPPER,
  RET_GLUE
};
} // namespace AVMISD

class AVMTargetLowering final : public TargetLowering {
public:
  AVMTargetLowering(const TargetMachine &TM, const AVMSubtarget &STI);

  MVT getPointerTy(const DataLayout &DL, uint32_t AS = 0) const override;
  EVT getTypeForExtReturn(LLVMContext &Context, EVT VT,
                          ISD::NodeType ExtendKind) const override;
  bool areJTsAllowed(const Function *) const override { return false; }
  const char *getTargetNodeName(unsigned Opcode) const override;
  bool allowsMisalignedMemoryAccesses(
      EVT VT, unsigned AddrSpace = 0, Align Alignment = Align(1),
      MachineMemOperand::Flags Flags = MachineMemOperand::MONone,
      unsigned *Fast = nullptr) const override;
  bool getPostIndexedAddressParts(SDNode *N, SDNode *Op, SDValue &Base,
                                  SDValue &Offset, ISD::MemIndexedMode &AM,
                                  SelectionDAG &DAG) const override;
  EVT getOptimalMemOpType(LLVMContext &Context, const MemOp &Op,
                          const AttributeList &FuncAttributes) const override;

private:
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  SDValue LowerBRCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSelect(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSelectCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSetCC(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerCall(CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context, const Type *RetTy) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;
};
} // namespace llvm

#endif
