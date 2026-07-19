//===-- AVMISelLowering.cpp - AVM DAG lowering --------------------------===//

#include "AVMISelLowering.h"
#include "AVMSubtarget.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

namespace {
constexpr MCPhysReg AVMArgUnits[] = {AVM::R4, AVM::R5, AVM::R6, AVM::R7};
constexpr MCPhysReg AVMArgPairs[] = {AVM::R4R5, AVM::R6R7};

template <typename ArgT> static unsigned getABIUnits(const ArgT &Arg) {
  if (Arg.Flags.isPointer() && Arg.Flags.getPointerAddrSpace() == 1)
    return 2;
  uint64_t Bits = Arg.ArgVT.getSizeInBits().getFixedValue();
  if (Bits <= 16)
    return 1;
  if (Bits <= 32)
    return 2;
  if (Bits <= 64)
    return 4;
  report_fatal_error("unsupported AVM scalar argument size");
}

template <typename ArgT> static unsigned getABIStackBytes(const ArgT &Arg) {
  if (Arg.Flags.isPointer() && Arg.Flags.getPointerAddrSpace() == 1)
    return 3;
  return divideCeil(Arg.ArgVT.getSizeInBits().getFixedValue(), uint64_t(8));
}

static MCPhysReg getAVMArgRegister(unsigned Unit, MVT VT) {
  if (VT == MVT::i32 || VT == MVT::f32) {
    assert(Unit % 2 == 0 && Unit < 4 && "unaligned AVM pair argument");
    return AVMArgPairs[Unit / 2];
  }
  assert((VT == MVT::i8 || VT == MVT::i16) && Unit < 4 &&
         "unsupported legalized AVM argument part");
  return AVMArgUnits[Unit];
}

template <typename ArgT>
static void analyzeAVMArguments(const SmallVectorImpl<ArgT> &Args,
                                CCState &State) {
  unsigned UnitCursor = 0;
  bool RegistersClosed = false;

  for (unsigned I = 0, E = Args.size(); I != E;) {
    unsigned J = I + 1;
    while (J != E && Args[J].OrigArgIndex == Args[I].OrigArgIndex)
      ++J;

    unsigned Units = getABIUnits(Args[I]);
    unsigned UnitAlign = Units == 1 ? 1 : 2;
    unsigned FirstUnit = alignTo(UnitCursor, UnitAlign);
    bool UseRegisters = !RegistersClosed && FirstUnit + Units <= 4 &&
                        (Units != 4 || FirstUnit == 0);

    int64_t StackBase = 0;
    if (UseRegisters) {
      UnitCursor = FirstUnit + Units;
    } else {
      RegistersClosed = true;
      StackBase = State.AllocateStack(getABIStackBytes(Args[I]), Align(1));
    }

    for (unsigned Part = I; Part != J; ++Part) {
      MVT ValVT = Args[Part].VT;
      if (UseRegisters) {
        unsigned Unit = FirstUnit + Args[Part].PartOffset / 2;
        MVT LocVT = ValVT == MVT::i8 ? MVT::i16 : ValVT;
        CCValAssign::LocInfo LocInfo = CCValAssign::Full;
        if (ValVT == MVT::i8)
          LocInfo =
              Args[Part].Flags.isSExt() ? CCValAssign::SExt : CCValAssign::ZExt;
        MCPhysReg Reg = State.AllocateReg(getAVMArgRegister(Unit, LocVT));
        assert(Reg && "AVM argument register was allocated twice");
        State.addLoc(CCValAssign::getReg(Part, ValVT, Reg, LocVT, LocInfo));
        continue;
      }

      MVT LocVT = ValVT;
      CCValAssign::LocInfo LocInfo = CCValAssign::Full;
      if (getABIStackBytes(Args[I]) == 1) {
        LocVT = MVT::i8;
        if (ValVT != MVT::i8)
          LocInfo = CCValAssign::Trunc;
      }
      State.addLoc(CCValAssign::getMem(
          Part, ValVT, StackBase + Args[Part].PartOffset, LocVT, LocInfo));
    }
    I = J;
  }
}

template <typename ArgT>
static bool analyzeAVMReturns(const SmallVectorImpl<ArgT> &Args,
                              CCState &State) {
  unsigned Unit = 0;
  for (unsigned I = 0; I != Args.size(); ++I) {
    MVT ValVT = Args[I].VT;
    uint64_t ABIBits =
        Args[I].Flags.isPointer() && Args[I].Flags.getPointerAddrSpace() == 1
            ? 32
            : Args[I].ArgVT.getSizeInBits().getFixedValue();
    MVT LocVT = ABIBits <= 16 ? MVT::i16 : ValVT;
    if ((ValVT != MVT::i8 && ValVT != MVT::i16 && ValVT != MVT::i32 &&
         ValVT != MVT::f32) ||
        (LocVT != MVT::i16 && LocVT != MVT::i32 && LocVT != MVT::f32) ||
        Unit + (LocVT.getStoreSize() / 2) > 4)
      return false;
    CCValAssign::LocInfo LocInfo =
        ValVT == LocVT ? CCValAssign::Full : CCValAssign::Trunc;
    MCPhysReg Reg = getAVMArgRegister(Unit, LocVT);
    State.addLoc(CCValAssign::getReg(I, ValVT, Reg, LocVT, LocInfo));
    Unit += LocVT.getStoreSize() / 2;
  }
  return true;
}

static SDValue canonicalizeNarrowOutgoing(SDValue Value,
                                          const ISD::OutputArg &Arg,
                                          const SDLoc &DL, SelectionDAG &DAG) {
  if (Value.getValueType() != MVT::i16 ||
      Arg.ArgVT.getSizeInBits().getFixedValue() >= 16)
    return Value;
  if (Arg.Flags.isSExt())
    return DAG.getNode(ISD::SIGN_EXTEND_INREG, DL, MVT::i16, Value,
                       DAG.getValueType(MVT::i8));
  return DAG.getNode(ISD::AND, DL, MVT::i16, Value,
                     DAG.getConstant(0xff, DL, MVT::i16));
}
} // namespace

AVMTargetLowering::AVMTargetLowering(const TargetMachine &TM,
                                     const AVMSubtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i16, &AVM::GPR16RegClass);
  addRegisterClass(MVT::i32, &AVM::GPR32RegClass);
  computeRegisterProperties(STI.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(AVM::SP);
  setBooleanContents(ZeroOrOneBooleanContent);
  setMinFunctionAlignment(Align(1));
  setPrefFunctionAlignment(Align(1));

  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::GlobalAddress, MVT::i16, Custom);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i16, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);

  setLoadExtAction({ISD::EXTLOAD, ISD::ZEXTLOAD, ISD::SEXTLOAD}, MVT::i16,
                   MVT::i8, Legal);
  setTruncStoreAction(MVT::i16, MVT::i8, Legal);
  setIndexedLoadAction(ISD::POST_INC, MVT::i8, Legal);
  setIndexedLoadAction(ISD::POST_INC, MVT::i16, Legal);
  setIndexedStoreAction(ISD::POST_INC, MVT::i8, Legal);
  setIndexedStoreAction(ISD::POST_INC, MVT::i16, Legal);

  MaxStoresPerMemset = MaxStoresPerMemcpy = MaxStoresPerMemmove = 8;
  MaxStoresPerMemsetOptSize = MaxStoresPerMemcpyOptSize =
      MaxStoresPerMemmoveOptSize = 4;
}

MVT AVMTargetLowering::getPointerTy(const DataLayout &DL, uint32_t AS) const {
  if (AS == 1)
    return MVT::i32;
  return TargetLowering::getPointerTy(DL, AS);
}

EVT AVMTargetLowering::getTypeForExtReturn(LLVMContext &, EVT VT,
                                           ISD::NodeType) const {
  return VT;
}

const char *AVMTargetLowering::getTargetNodeName(unsigned Opcode) const {
  if (Opcode == AVMISD::CALL)
    return "AVMISD::CALL";
  if (Opcode == AVMISD::LOAD24)
    return "AVMISD::LOAD24";
  if (Opcode == AVMISD::STORE24)
    return "AVMISD::STORE24";
  if (Opcode == AVMISD::WRAPPER)
    return "AVMISD::WRAPPER";
  if (Opcode == AVMISD::RET_GLUE)
    return "AVMISD::RET_GLUE";
  return nullptr;
}

bool AVMTargetLowering::allowsMisalignedMemoryAccesses(EVT VT,
                                                       unsigned AddrSpace,
                                                       Align,
                                                       MachineMemOperand::Flags,
                                                       unsigned *Fast) const {
  if (AddrSpace != 0 || (VT != MVT::i8 && VT != MVT::i16 && VT != MVT::i32))
    return false;
  if (Fast)
    *Fast = 1;
  return true;
}

bool AVMTargetLowering::getPostIndexedAddressParts(SDNode *N, SDNode *Op,
                                                   SDValue &Base,
                                                   SDValue &Offset,
                                                   ISD::MemIndexedMode &AM,
                                                   SelectionDAG &DAG) const {
  EVT MemoryVT;
  SDValue Pointer;
  unsigned AddressSpace;
  if (const auto *Load = dyn_cast<LoadSDNode>(N)) {
    MemoryVT = Load->getMemoryVT();
    Pointer = Load->getBasePtr();
    AddressSpace = Load->getAddressSpace();
  } else if (const auto *Store = dyn_cast<StoreSDNode>(N)) {
    MemoryVT = Store->getMemoryVT();
    Pointer = Store->getBasePtr();
    AddressSpace = Store->getAddressSpace();
  } else {
    return false;
  }
  if (AddressSpace != 0 || (MemoryVT != MVT::i8 && MemoryVT != MVT::i16) ||
      Op->getOpcode() != ISD::ADD)
    return false;
  const auto *Increment = dyn_cast<ConstantSDNode>(Op->getOperand(1));
  int64_t Width = MemoryVT == MVT::i8 ? 1 : 2;
  if (!Increment || Increment->getSExtValue() != Width ||
      Pointer != Op->getOperand(0))
    return false;
  Base = Pointer;
  Offset = DAG.getConstant(Width, SDLoc(N), MVT::i16);
  AM = ISD::POST_INC;
  return true;
}

EVT AVMTargetLowering::getOptimalMemOpType(LLVMContext &, const MemOp &,
                                           const AttributeList &) const {
  // Byte stores preserve memset's value without constructing an i16/i32 splat,
  // and make overlap-safe memmove expansion independent of word alignment.
  return MVT::i8;
}

SDValue AVMTargetLowering::LowerGlobalAddress(SDValue Op,
                                              SelectionDAG &DAG) const {
  const auto *GA = cast<GlobalAddressSDNode>(Op);
  SDLoc DL(Op);
  SDValue Target = DAG.getTargetGlobalAddress(
      GA->getGlobal(), DL, MVT::i16, GA->getOffset(), GA->getTargetFlags());
  return DAG.getNode(AVMISD::WRAPPER, DL, MVT::i16, Target);
}

SDValue AVMTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  if (Op.getOpcode() == ISD::GlobalAddress)
    return LowerGlobalAddress(Op, DAG);
  llvm_unreachable("unexpected AVM custom-lowered operation");
}

SDValue AVMTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  if (CallConv != CallingConv::C || IsVarArg)
    report_fatal_error("unsupported AVM calling convention");

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  SmallVector<CCValAssign, 8> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  analyzeAVMArguments(Ins, CCInfo);

  MachineRegisterInfo &MRI = MF.getRegInfo();
  SmallVector<SDValue, 4> LoadChains;
  for (const CCValAssign &VA : ArgLocs) {
    unsigned I = VA.getValNo();
    if (VA.isRegLoc()) {
      const TargetRegisterClass *RC =
          VA.getLocVT() == MVT::i32 ? &AVM::GPR32RegClass : &AVM::GPR16RegClass;
      Register VReg = MRI.createVirtualRegister(RC);
      MRI.addLiveIn(VA.getLocReg(), VReg);
      SDValue Value = DAG.getCopyFromReg(Chain, DL, VReg, VA.getLocVT());
      if (VA.getLocVT() == MVT::i16 &&
          Ins[I].ArgVT.getSizeInBits().getFixedValue() < 16) {
        unsigned AssertOp =
            Ins[I].Flags.isSExt() ? ISD::AssertSext : ISD::AssertZext;
        Value = DAG.getNode(AssertOp, DL, MVT::i16, Value,
                            DAG.getValueType(MVT::i8));
      }
      InVals.push_back(Value);
      continue;
    }

    assert(VA.isMemLoc() && "invalid AVM formal-argument location");
    bool IsProgramPointer =
        Ins[I].Flags.isPointer() && Ins[I].Flags.getPointerAddrSpace() == 1;
    unsigned Size = IsProgramPointer ? 3 : VA.getLocVT().getStoreSize();
    int FI = MFI.CreateFixedObject(Size, VA.getLocMemOffset() + 3, true);
    SDValue FIN = DAG.getFrameIndex(FI, MVT::i16);
    MachinePointerInfo PtrInfo = MachinePointerInfo::getFixedStack(MF, FI);
    SDValue Load;
    if (IsProgramPointer) {
      Load = DAG.getNode(AVMISD::LOAD24, DL,
                         DAG.getVTList(MVT::i32, MVT::Other), Chain, FIN);
    } else if (VA.getLocVT() == VA.getValVT()) {
      Load = DAG.getLoad(VA.getValVT(), DL, Chain, FIN, PtrInfo, Align(1));
    } else {
      ISD::LoadExtType Ext =
          Ins[I].Flags.isSExt() ? ISD::SEXTLOAD : ISD::ZEXTLOAD;
      Load = DAG.getExtLoad(Ext, DL, VA.getValVT(), Chain, FIN, PtrInfo,
                            VA.getLocVT(), Align(1));
    }
    InVals.push_back(Load);
    LoadChains.push_back(Load.getValue(1));
  }
  if (!LoadChains.empty()) {
    LoadChains.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, LoadChains);
  }
  return Chain;
}

SDValue AVMTargetLowering::LowerCall(CallLoweringInfo &CLI,
                                     SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  const SDLoc &DL = CLI.DL;
  if (CLI.CallConv != CallingConv::C || CLI.IsVarArg)
    report_fatal_error("unsupported AVM calling convention");
  CLI.IsTailCall = false;

  SmallVector<CCValAssign, 8> ArgLocs;
  CCState CCInfo(CLI.CallConv, CLI.IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  analyzeAVMArguments(CLI.Outs, CCInfo);

  unsigned NumBytes = CCInfo.getStackSize();
  SDValue Chain = DAG.getCALLSEQ_START(CLI.Chain, NumBytes, 0, DL);
  SmallVector<std::pair<MCPhysReg, SDValue>, 4> RegsToPass;
  SmallVector<SDValue, 4> StoreChains;

  for (const CCValAssign &VA : ArgLocs) {
    unsigned I = VA.getValNo();
    SDValue Arg = CLI.OutVals[I];
    if (VA.isRegLoc()) {
      Arg = canonicalizeNarrowOutgoing(Arg, CLI.Outs[I], DL, DAG);
      RegsToPass.emplace_back(VA.getLocReg(), Arg);
      continue;
    }

    assert(VA.isMemLoc() && "invalid AVM call-argument location");
    bool IsProgramPointer = CLI.Outs[I].Flags.isPointer() &&
                            CLI.Outs[I].Flags.getPointerAddrSpace() == 1;
    if (VA.getValVT() != MVT::i16 && VA.getValVT() != MVT::i32)
      report_fatal_error(
          "stack lowering for this AVM scalar type is not implemented yet");
    SDValue Ptr =
        DAG.getNode(ISD::ADD, DL, MVT::i16, DAG.getRegister(AVM::SP, MVT::i16),
                    DAG.getConstant(VA.getLocMemOffset(), DL, MVT::i16));
    MachinePointerInfo PtrInfo = MachinePointerInfo::getStack(
        DAG.getMachineFunction(), VA.getLocMemOffset());
    SDValue Store;
    if (IsProgramPointer)
      Store = DAG.getNode(AVMISD::STORE24, DL, MVT::Other, Chain, Arg,
                          DAG.getConstant(VA.getLocMemOffset(), DL, MVT::i16));
    else if (VA.getLocVT() == MVT::i8)
      Store =
          DAG.getTruncStore(Chain, DL, Arg, Ptr, PtrInfo, MVT::i8, Align(1));
    else
      Store = DAG.getStore(Chain, DL, Arg, Ptr, PtrInfo, Align(1));
    StoreChains.push_back(Store);
  }
  if (!StoreChains.empty()) {
    StoreChains.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, StoreChains);
  }

  SDValue Glue;
  for (const auto &[Reg, Value] : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, DL, Reg, Value, Glue);
    Glue = Chain.getValue(1);
  }

  SDValue Callee = CLI.Callee;
  if (const auto *G = dyn_cast<GlobalAddressSDNode>(Callee))
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), DL, MVT::i32);
  else if (const auto *E = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), MVT::i32);
  else if (Callee.getValueType() != MVT::i32)
    Callee = DAG.getZExtOrTrunc(Callee, DL, MVT::i32);

  SmallVector<SDValue, 10> Ops = {Chain, Callee};
  for (const auto &[Reg, Value] : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg, Value.getValueType()));
  const TargetRegisterInfo *TRI =
      DAG.getMachineFunction().getSubtarget().getRegisterInfo();
  const uint32_t *Mask =
      TRI->getCallPreservedMask(DAG.getMachineFunction(), CLI.CallConv);
  assert(Mask && "missing AVM call-preserved mask");
  Ops.push_back(DAG.getRegisterMask(Mask));
  if (Glue)
    Ops.push_back(Glue);

  Chain =
      DAG.getNode(AVMISD::CALL, DL, DAG.getVTList(MVT::Other, MVT::Glue), Ops);
  Glue = Chain.getValue(1);
  Chain = DAG.getCALLSEQ_END(Chain, NumBytes, 0, Glue, DL);
  Glue = Chain.getValue(1);

  SmallVector<CCValAssign, 4> RVLocs;
  CCState RVInfo(CLI.CallConv, CLI.IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  if (!analyzeAVMReturns(CLI.Ins, RVInfo))
    report_fatal_error("unsupported AVM call return type");
  for (const CCValAssign &VA : RVLocs) {
    SDValue Copy =
        DAG.getCopyFromReg(Chain, DL, VA.getLocReg(), VA.getLocVT(), Glue);
    InVals.push_back(Copy);
    Chain = Copy.getValue(1);
    Glue = Copy.getValue(2);
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
  return analyzeAVMReturns(Outs, CCInfo);
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
  if (!analyzeAVMReturns(Outs, CCInfo))
    report_fatal_error("unsupported AVM return type");

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  for (unsigned I = 0; I != RVLocs.size(); ++I) {
    const CCValAssign &VA = RVLocs[I];
    assert(VA.isRegLoc() && "AVM returns must use registers");
    SDValue Value = OutVals[VA.getValNo()];
    if (VA.getLocInfo() == CCValAssign::Trunc)
      Value = DAG.getNode(ISD::TRUNCATE, DL, VA.getLocVT(), Value);
    else
      assert(VA.getLocInfo() == CCValAssign::Full &&
             "unsupported AVM return conversion");
    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), Value, Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain;
  if (Glue)
    RetOps.push_back(Glue);
  return DAG.getNode(AVMISD::RET_GLUE, DL, MVT::Other, RetOps);
}
