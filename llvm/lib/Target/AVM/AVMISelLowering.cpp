//===-- AVMISelLowering.cpp - AVM DAG lowering --------------------------===//

#include "AVMISelLowering.h"
#include "AVMSubtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#include "AVMGenCallingConv.inc"

AVMTargetLowering::AVMTargetLowering(const TargetMachine &TM,
                                     const AVMSubtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i16, &AVM::GPR16RegClass);
  computeRegisterProperties(STI.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(AVM::SP);
  setBooleanContents(ZeroOrOneBooleanContent);
  setMinFunctionAlignment(Align(1));
  setPrefFunctionAlignment(Align(1));

  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i16, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
}

const char *AVMTargetLowering::getTargetNodeName(unsigned Opcode) const {
  if (Opcode == AVMISD::RET_GLUE)
    return "AVMISD::RET_GLUE";
  return nullptr;
}

SDValue AVMTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  if (CallConv != CallingConv::C || IsVarArg)
    report_fatal_error(
        "unsupported AVM calling convention in minimal leaf codegen");

  SmallVector<CCValAssign, 4> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_AVM);

  MachineRegisterInfo &MRI = DAG.getMachineFunction().getRegInfo();
  for (const CCValAssign &VA : ArgLocs) {
    if (!VA.isRegLoc() || VA.getLocVT() != MVT::i16 ||
        VA.getLocInfo() != CCValAssign::Full)
      report_fatal_error("only register i16 AVM arguments are implemented");

    Register VReg = MRI.createVirtualRegister(&AVM::GPR16RegClass);
    MRI.addLiveIn(VA.getLocReg(), VReg);
    InVals.push_back(DAG.getCopyFromReg(Chain, DL, VReg, MVT::i16));
  }
  return Chain;
}

bool AVMTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *) const {
  if (CallConv != CallingConv::C || IsVarArg)
    return false;
  SmallVector<CCValAssign, 2> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);
  return CCInfo.CheckReturn(Outs, RetCC_AVM);
}

SDValue
AVMTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::OutputArg> &Outs,
                               const SmallVectorImpl<SDValue> &OutVals,
                               const SDLoc &DL, SelectionDAG &DAG) const {
  if (CallConv != CallingConv::C || IsVarArg)
    report_fatal_error("unsupported AVM return calling convention");

  SmallVector<CCValAssign, 2> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_AVM);

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  for (unsigned I = 0; I != RVLocs.size(); ++I) {
    const CCValAssign &VA = RVLocs[I];
    if (!VA.isRegLoc() || VA.getLocVT() != MVT::i16)
      report_fatal_error("only i16 AVM returns are implemented");
    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), OutVals[I], Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain;
  if (Glue)
    RetOps.push_back(Glue);
  return DAG.getNode(AVMISD::RET_GLUE, DL, MVT::Other, RetOps);
}
