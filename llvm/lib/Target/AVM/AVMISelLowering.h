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
  FCMP,
  FCLASS,
  LOAD24,
  NORMALIZE_PROGPTR,
  PROG_WRAPPER,
  STORE24,
  TST8,
  TST16,
  WRAPPER,
  SHL32_16,
  SRL32_16,
  SRA32_16,
  RET_GLUE
};
} // namespace AVMISD

class AVMTargetLowering final : public TargetLowering {
public:
  AVMTargetLowering(const TargetMachine &TM, const AVMSubtarget &STI);

  MVT getPointerTy(const DataLayout &DL, uint32_t AS = 0) const override;
  MVT getScalarShiftAmountTy(const DataLayout &, EVT LHSTy) const override {
    return LHSTy.getSimpleVT();
  }
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
  EVT getAsmOperandValueType(const DataLayout &DL, Type *Ty,
                             bool AllowUnknown = false) const override {
    EVT VT = TargetLowering::getAsmOperandValueType(DL, Ty, AllowUnknown);
    return VT == MVT::i8 ? EVT(MVT::i16) : VT;
  }

  ConstraintType getConstraintType(StringRef Constraint) const override;
  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                               StringRef Constraint, MVT VT) const override;
  ConstraintWeight
  getSingleConstraintMatchWeight(AsmOperandInfo &Info,
                                 const char *Constraint) const override;
  void LowerAsmOperandForConstraint(SDValue Op, StringRef Constraint,
                                    std::vector<SDValue> &Ops,
                                    SelectionDAG &DAG) const override;

  AtomicExpansionKind shouldExpandAtomicLoadInIR(LoadInst *LI) const override;
  AtomicExpansionKind shouldExpandAtomicStoreInIR(StoreInst *SI) const override;
  AtomicExpansionKind
  shouldExpandAtomicCmpXchgInIR(AtomicCmpXchgInst *CI) const override;
  AtomicExpansionKind
  shouldExpandAtomicRMWInIR(AtomicRMWInst *AI) const override;

  bool isFPImmLegal(const APFloat &Imm, EVT VT,
                    bool ForCodeSize) const override;

private:
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  SDValue LowerBRCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerISFPClass(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSelect(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSelectCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSetCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;

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
