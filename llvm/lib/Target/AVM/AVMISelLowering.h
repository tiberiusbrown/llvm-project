//===-- AVMISelLowering.h - AVM DAG lowering interface --------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMISELLOWERING_H
#define LLVM_LIB_TARGET_AVM_AVMISELLOWERING_H

#include "AVM.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class AVMSubtarget;

namespace AVMISD {
enum NodeType : unsigned { FIRST_NUMBER = ISD::BUILTIN_OP_END, RET_GLUE };
} // namespace AVMISD

class AVMTargetLowering final : public TargetLowering {
public:
  AVMTargetLowering(const TargetMachine &TM, const AVMSubtarget &STI);

  const char *getTargetNodeName(unsigned Opcode) const override;

private:
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
