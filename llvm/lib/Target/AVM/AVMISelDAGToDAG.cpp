//===-- AVMISelDAGToDAG.cpp - AVM DAG instruction selector ---------------===//

#include "AVM.h"
#include "AVMTargetMachine.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/MathExtras.h"

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

  enum class ByteExtension { None, Unsigned, Signed };

  ByteExtension classifyByteValue(SDValue Value, SDValue &ByteValue) const {
    if (Value.getOpcode() == ISD::AssertZext &&
        cast<VTSDNode>(Value.getOperand(1))->getVT() == MVT::i8) {
      ByteValue = Value.getOperand(0);
      return ByteExtension::Unsigned;
    }
    if (Value.getOpcode() == ISD::SIGN_EXTEND_INREG &&
        cast<VTSDNode>(Value.getOperand(1))->getVT() == MVT::i8) {
      ByteValue = Value.getOperand(0);
      return ByteExtension::Signed;
    }
    if (Value.getOpcode() == ISD::AND) {
      if (const auto *Mask = dyn_cast<ConstantSDNode>(Value.getOperand(1));
          Mask && Mask->getZExtValue() == 0xff) {
        ByteValue = Value.getOperand(0);
        return ByteExtension::Unsigned;
      }
    }
    if (const auto *Load = dyn_cast<LoadSDNode>(Value);
        Load && Load->getMemoryVT() == MVT::i8) {
      ByteValue = Value;
      return Load->getExtensionType() == ISD::SEXTLOAD
                 ? ByteExtension::Signed
                 : ByteExtension::Unsigned;
    }
    ByteValue = Value;
    return ByteExtension::None;
  }

  bool getKnownShiftCount(SDValue CountValue, unsigned &Count) const {
    if (const auto *C = dyn_cast<ConstantSDNode>(CountValue)) {
      Count = C->getZExtValue();
      return true;
    }
    KnownBits Known = CurDAG->computeKnownBits(CountValue);
    if (!Known.isConstant())
      return false;
    Count = Known.getConstant().getZExtValue();
    return true;
  }

  bool selectAVMCompare(SDNode *Node) {
    unsigned Opcode;
    switch (Node->getOpcode()) {
    case AVMISD::CMP:
      Opcode = AVM::CMP16_PSEUDO;
      break;
    case AVMISD::CMPI:
      Opcode = AVM::CMPIS8_PSEUDO;
      break;
    case AVMISD::TST8:
      Opcode = AVM::TST8_PSEUDO;
      break;
    case AVMISD::TST16:
      Opcode = AVM::TST16_PSEUDO;
      break;
    default:
      return false;
    }
    SmallVector<SDValue, 2> Ops(Node->op_begin(), Node->op_end());
    CurDAG->SelectNodeTo(Node, Opcode, MVT::Glue, Ops);
    return true;
  }

  bool selectAVMBranchCC(SDNode *Node) {
    if (Node->getOpcode() != AVMISD::BR_CC)
      return false;
    SDValue Ops[] = {Node->getOperand(1), Node->getOperand(2),
                     Node->getOperand(0), Node->getOperand(3)};
    CurDAG->SelectNodeTo(Node, AVM::BR_CC_PSEUDO, MVT::Other, Ops);
    return true;
  }

  bool selectAVMCSet(SDNode *Node) {
    if (Node->getOpcode() != AVMISD::CSET)
      return false;
    SDValue Ops[] = {Node->getOperand(0), Node->getOperand(1)};
    CurDAG->SelectNodeTo(Node, AVM::CSET_PSEUDO, Node->getValueType(0), Ops);
    return true;
  }

  bool selectAVMCMov(SDNode *Node) {
    if (Node->getOpcode() != AVMISD::CMOV)
      return false;
    unsigned Opcode = Node->getValueType(0) == MVT::i32 ? AVM::CMOV32_PSEUDO
                                                        : AVM::CMOV16_PSEUDO;
    SDValue Ops[] = {Node->getOperand(0), Node->getOperand(1),
                     Node->getOperand(2), Node->getOperand(3)};
    CurDAG->SelectNodeTo(Node, Opcode, Node->getValueType(0), Ops);
    return true;
  }

  bool selectUnconditionalBranch(SDNode *Node) {
    if (Node->getOpcode() != ISD::BR)
      return false;
    SDValue Ops[] = {Node->getOperand(1), Node->getOperand(0)};
    CurDAG->SelectNodeTo(Node, AVM::JMP_PSEUDO, MVT::Other, Ops);
    return true;
  }

  bool selectAddSub(SDNode *Node) {
    if (Node->getValueType(0) != MVT::i16)
      return false;
    bool IsAdd = Node->getOpcode() == ISD::ADD;
    bool IsSub = Node->getOpcode() == ISD::SUB;
    if (!IsAdd && !IsSub)
      return false;

    SDValue Value = Node->getOperand(0);
    const auto *C = dyn_cast<ConstantSDNode>(Node->getOperand(1));
    if (IsAdd && !C) {
      C = dyn_cast<ConstantSDNode>(Value);
      if (C)
        Value = Node->getOperand(1);
    }
    if (!C)
      return false;

    int64_t Imm = C->getSExtValue();
    unsigned Opcode = 0;
    if ((IsAdd && Imm == 1))
      Opcode = AVM::INC16_PSEUDO;
    else if ((IsAdd && Imm == -1) || (IsSub && Imm == 1))
      Opcode = AVM::DEC16_PSEUDO;
    else if (IsAdd && isInt<8>(Imm))
      Opcode = AVM::ADDIS8_PSEUDO;
    else
      return false;

    if (Opcode == AVM::ADDIS8_PSEUDO) {
      SDValue Ops[] = {
          Value, CurDAG->getSignedTargetConstant(Imm, SDLoc(Node), MVT::i16)};
      CurDAG->SelectNodeTo(Node, Opcode, MVT::i16, Ops);
    } else {
      CurDAG->SelectNodeTo(Node, Opcode, MVT::i16, Value);
    }
    return true;
  }

  bool selectMultiply(SDNode *Node) {
    if (Node->getOpcode() != ISD::MUL || Node->getValueType(0) != MVT::i16)
      return false;

    SDValue LeftByte;
    SDValue RightByte;
    ByteExtension Left = classifyByteValue(Node->getOperand(0), LeftByte);
    ByteExtension Right = classifyByteValue(Node->getOperand(1), RightByte);
    unsigned Opcode = AVM::MUL16_PSEUDO;
    SDValue LHS = Node->getOperand(0);
    SDValue RHS = Node->getOperand(1);

    if (Left == ByteExtension::Unsigned && Right == ByteExtension::Unsigned) {
      Opcode = AVM::MULU8W_PSEUDO;
      LHS = LeftByte;
      RHS = RightByte;
    } else if (Left == ByteExtension::Signed &&
               Right == ByteExtension::Signed) {
      Opcode = AVM::MULS8W_PSEUDO;
      LHS = LeftByte;
      RHS = RightByte;
    } else if (Left == ByteExtension::Signed &&
               Right == ByteExtension::Unsigned) {
      Opcode = AVM::MULSU8W_PSEUDO;
      LHS = LeftByte;
      RHS = RightByte;
    } else if (Left == ByteExtension::Unsigned &&
               Right == ByteExtension::Signed) {
      Opcode = AVM::MULSU8W_PSEUDO;
      LHS = RightByte;
      RHS = LeftByte;
    }

    SDValue Ops[] = {LHS, RHS};
    CurDAG->SelectNodeTo(Node, Opcode, MVT::i16, Ops);
    return true;
  }

  bool selectDivision(SDNode *Node) {
    if (Node->getValueType(0) != MVT::i16)
      return false;
    unsigned Opcode;
    switch (Node->getOpcode()) {
    case ISD::UDIV:
      Opcode = AVM::UDIV16_PSEUDO;
      break;
    case ISD::UREM:
      Opcode = AVM::UREM16_PSEUDO;
      break;
    case ISD::SDIV:
      Opcode = AVM::SDIV16_PSEUDO;
      break;
    case ISD::SREM:
      Opcode = AVM::SREM16_PSEUDO;
      break;
    default:
      return false;
    }
    SDValue Ops[] = {Node->getOperand(0), Node->getOperand(1)};
    CurDAG->SelectNodeTo(Node, Opcode, MVT::i16, Ops);
    return true;
  }

  bool selectShift(SDNode *Node) {
    if (Node->getValueType(0) != MVT::i16)
      return false;
    unsigned ShiftOpcode = Node->getOpcode();
    if (ShiftOpcode != ISD::SHL && ShiftOpcode != ISD::SRL &&
        ShiftOpcode != ISD::SRA)
      return false;

    SDValue Value = Node->getOperand(0);
    SDValue CountValue = Node->getOperand(1);
    unsigned Count;
    if (getKnownShiftCount(CountValue, Count)) {
      if (Count > 15)
        return false;
      if (Count == 0) {
        ReplaceUses(SDValue(Node, 0), Value);
        CurDAG->RemoveDeadNode(Node);
        return true;
      }

      bool OptSize = CurDAG->getMachineFunction().getFunction().hasOptSize();
      unsigned Opcode;
      if (ShiftOpcode == ISD::SHL)
        Opcode = OptSize ? AVM::LSL16I_PSEUDO
                         : (Count <= 3 ? AVM::SHL16_SMALL_PSEUDO
                                       : AVM::LSL16I_PSEUDO);
      else if (ShiftOpcode == ISD::SRL)
        Opcode =
            !OptSize && Count == 1 ? AVM::LSR16_1_PSEUDO : AVM::LSR16I_PSEUDO;
      else
        Opcode =
            !OptSize && Count == 1 ? AVM::ASR16_1_PSEUDO : AVM::ASR16I_PSEUDO;

      if (Opcode == AVM::LSR16_1_PSEUDO || Opcode == AVM::ASR16_1_PSEUDO) {
        CurDAG->SelectNodeTo(Node, Opcode, MVT::i16, Value);
      } else {
        SDValue Ops[] = {
            Value, CurDAG->getTargetConstant(Count, SDLoc(Node), MVT::i16)};
        CurDAG->SelectNodeTo(Node, Opcode, MVT::i16, Ops);
      }
      return true;
    }

    unsigned Opcode = ShiftOpcode == ISD::SHL   ? AVM::SHL16V_PSEUDO
                      : ShiftOpcode == ISD::SRL ? AVM::LSR16V_PSEUDO
                                                : AVM::ASR16V_PSEUDO;
    SDValue Ops[] = {Value, CountValue};
    CurDAG->SelectNodeTo(Node, Opcode, MVT::i16, Ops);
    return true;
  }

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

  bool matchDataSymbol(SDValue Address, SDValue &Symbol) {
    int64_t ExtraOffset = 0;
    if (Address.getOpcode() == ISD::ADD) {
      const auto *Offset = dyn_cast<ConstantSDNode>(Address.getOperand(1));
      if (!Offset)
        return false;
      ExtraOffset = Offset->getSExtValue();
      Address = Address.getOperand(0);
    }
    if (Address.getOpcode() != AVMISD::WRAPPER)
      return false;
    const auto *GA = dyn_cast<GlobalAddressSDNode>(Address.getOperand(0));
    if (!GA)
      return false;
    Symbol = CurDAG->getTargetGlobalAddress(
        GA->getGlobal(), SDLoc(Address), MVT::i16,
        GA->getOffset() + ExtraOffset, GA->getTargetFlags());
    return true;
  }

  SDValue selectSignExtend8(SDValue Value, const SDLoc &DL) {
    SDNode *Sext =
        CurDAG->getMachineNode(AVM::SEXT8_PSEUDO, DL, MVT::i16, Value);
    return SDValue(Sext, 0);
  }

  bool selectDataLoad(SDNode *Node) {
    const auto *Load = cast<LoadSDNode>(Node);
    if (Load->getAddressSpace() != 0)
      return false;

    EVT MemoryVT = Load->getMemoryVT();
    if (MemoryVT != MVT::i8 && MemoryVT != MVT::i16 && MemoryVT != MVT::i32)
      return false;
    bool IsByte = MemoryVT == MVT::i8;
    bool IsPair = MemoryVT == MVT::i32;
    if (Load->getValueType(0) != (IsPair ? MVT::i32 : MVT::i16))
      return false;
    bool IsSigned = IsByte && Load->getExtensionType() == ISD::SEXTLOAD;
    bool IsPostInc = Load->getAddressingMode() == ISD::POST_INC;

    SDValue Address = Load->getBasePtr();
    unsigned Opcode;
    SmallVector<EVT, 3> ResultVTs;
    SmallVector<SDValue, 3> Ops;
    SDValue Symbol;

    if (!IsPair && Load->getAddressingMode() == ISD::UNINDEXED &&
        matchDataSymbol(Address, Symbol)) {
      Opcode = IsByte ? AVM::ABS_LOAD8U_PSEUDO : AVM::ABS_LOAD16_PSEUDO;
      ResultVTs = {MVT::i16, MVT::Other};
      Ops = {Symbol, Load->getChain()};
    } else if (IsPostInc && !IsPair) {
      const auto *Increment = dyn_cast<ConstantSDNode>(Load->getOffset());
      if (!Increment || Increment->getSExtValue() != (IsByte ? 1 : 2))
        return false;
      Opcode = IsByte ? AVM::LOAD8U_POST_PSEUDO : AVM::LOAD16_POST_PSEUDO;
      ResultVTs = {MVT::i16, MVT::i16, MVT::Other};
      Ops = {Address, Load->getChain()};
    } else if (Load->getAddressingMode() == ISD::UNINDEXED) {
      Opcode = IsPair ? AVM::LOAD32_PSEUDO
                      : (IsByte ? AVM::LOAD8U_PSEUDO : AVM::LOAD16_PSEUDO);
      ResultVTs = {IsPair ? EVT(MVT::i32) : EVT(MVT::i16), MVT::Other};
      Ops = {Address, Load->getChain()};
    } else {
      return false;
    }

    SDNode *Result =
        CurDAG->getMachineNode(Opcode, SDLoc(Node), ResultVTs, Ops);
    CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                           {Load->getMemOperand()});
    SDValue Value(Result, 0);
    if (IsSigned)
      Value = selectSignExtend8(Value, SDLoc(Node));
    ReplaceUses(SDValue(Node, 0), Value);
    if (IsPostInc) {
      ReplaceUses(SDValue(Node, 1), SDValue(Result, 1));
      ReplaceUses(SDValue(Node, 2), SDValue(Result, 2));
    } else {
      ReplaceUses(SDValue(Node, 1), SDValue(Result, 1));
    }
    CurDAG->RemoveDeadNode(Node);
    return true;
  }

  bool selectDataStore(SDNode *Node) {
    const auto *Store = cast<StoreSDNode>(Node);
    if (Store->getAddressSpace() != 0)
      return false;

    EVT MemoryVT = Store->getMemoryVT();
    if (MemoryVT != MVT::i8 && MemoryVT != MVT::i16 && MemoryVT != MVT::i32)
      return false;
    bool IsByte = MemoryVT == MVT::i8;
    bool IsPair = MemoryVT == MVT::i32;
    bool IsPostInc = Store->getAddressingMode() == ISD::POST_INC;

    SDValue Address = Store->getBasePtr();
    unsigned Opcode;
    SmallVector<EVT, 2> ResultVTs;
    SmallVector<SDValue, 4> Ops;
    SDValue Symbol;
    if (!IsPair && Store->getAddressingMode() == ISD::UNINDEXED &&
        matchDataSymbol(Address, Symbol)) {
      Opcode = IsByte ? AVM::ABS_STORE8_PSEUDO : AVM::ABS_STORE16_PSEUDO;
      ResultVTs = {MVT::Other};
      Ops = {Symbol, Store->getValue(), Store->getChain()};
    } else if (IsPostInc && !IsPair) {
      const auto *Increment = dyn_cast<ConstantSDNode>(Store->getOffset());
      if (!Increment || Increment->getSExtValue() != (IsByte ? 1 : 2))
        return false;
      Opcode = IsByte ? AVM::STORE8_POST_PSEUDO : AVM::STORE16_POST_PSEUDO;
      ResultVTs = {MVT::i16, MVT::Other};
      Ops = {Address, Store->getValue(), Store->getChain()};
    } else if (Store->getAddressingMode() == ISD::UNINDEXED) {
      Opcode = IsPair ? AVM::STORE32_PSEUDO
                      : (IsByte ? AVM::STORE8_PSEUDO : AVM::STORE16_PSEUDO);
      ResultVTs = {MVT::Other};
      Ops = {Address, Store->getValue(), Store->getChain()};
    } else {
      return false;
    }

    SDNode *Result =
        CurDAG->getMachineNode(Opcode, SDLoc(Node), ResultVTs, Ops);
    CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                           {Store->getMemOperand()});
    if (IsPostInc) {
      ReplaceUses(SDValue(Node, 0), SDValue(Result, 0));
      ReplaceUses(SDValue(Node, 1), SDValue(Result, 1));
    } else {
      ReplaceUses(SDValue(Node, 0), SDValue(Result, 0));
    }
    CurDAG->RemoveDeadNode(Node);
    return true;
  }

  bool selectDataAddress(SDNode *Node) {
    if (Node->getOpcode() != AVMISD::WRAPPER)
      return false;
    CurDAG->SelectNodeTo(Node, AVM::DATA_ADDR_PSEUDO, MVT::i16,
                         Node->getOperand(0));
    return true;
  }

  bool selectZeroExtend8(SDNode *Node) {
    if (Node->getOpcode() != ISD::AND || Node->getValueType(0) != MVT::i16)
      return false;
    SDValue Value = Node->getOperand(0);
    const auto *Mask = dyn_cast<ConstantSDNode>(Node->getOperand(1));
    if (!Mask) {
      Mask = dyn_cast<ConstantSDNode>(Value);
      Value = Node->getOperand(1);
    }
    if (!Mask || Mask->getZExtValue() != 0xff)
      return false;

    if (Value.getOpcode() == ISD::MUL) {
      SDValue Ops[] = {Value.getOperand(0), Value.getOperand(1)};
      CurDAG->SelectNodeTo(Node, AVM::MUL8_PSEUDO, MVT::i16, Ops);
      return true;
    }

    CurDAG->SelectNodeTo(Node, AVM::ZEXT8_PSEUDO, MVT::i16, Value);
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
    case AVMISD::BR_CC:
      selectAVMBranchCC(Node);
      return;
    case AVMISD::CALL:
      selectCall(Node);
      return;
    case AVMISD::CMOV:
      selectAVMCMov(Node);
      return;
    case AVMISD::CMP:
    case AVMISD::CMPI:
    case AVMISD::TST8:
    case AVMISD::TST16:
      selectAVMCompare(Node);
      return;
    case AVMISD::CSET:
      selectAVMCSet(Node);
      return;
    case AVMISD::LOAD24:
      if (selectLoad24(Node))
        return;
      break;
    case AVMISD::STORE24:
      selectStore24(Node);
      return;
    case ISD::LOAD:
      if (selectStackLoad(Node) || selectDataLoad(Node))
        return;
      break;
    case ISD::STORE:
      if (selectStackStore(Node) || selectOutgoingStore(Node) ||
          selectDataStore(Node))
        return;
      break;
    case ISD::AND:
      if (selectZeroExtend8(Node))
        return;
      break;
    case ISD::ADD:
      if (selectFrameAddress(Node))
        return;
      if (selectAddSub(Node))
        return;
      break;
    case ISD::SUB:
      if (selectAddSub(Node))
        return;
      break;
    case ISD::MUL:
      if (selectMultiply(Node))
        return;
      break;
    case ISD::UDIV:
    case ISD::UREM:
    case ISD::SDIV:
    case ISD::SREM:
      if (selectDivision(Node))
        return;
      break;
    case ISD::SHL:
    case ISD::SRL:
    case ISD::SRA:
      if (selectShift(Node))
        return;
      break;
    case ISD::BR:
      selectUnconditionalBranch(Node);
      return;
    case ISD::FrameIndex:
      selectFrameIndex(Node);
      return;
    case AVMISD::WRAPPER:
      selectDataAddress(Node);
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
