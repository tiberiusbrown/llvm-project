//===-- AVMInstrInfo.td - AVM MC instruction definitions ---*- tablegen -*-===//

// The current opcode map is introduced in a later change.  These are the
// already-verified, direct far transfers whose encodings match docs/arch.md.
def AVMImm24 : Operand<i32>;
def AVMImm8 : Operand<i32>;
def AVMImm4 : Operand<i32>;
def AVMImm16 : Operand<i32>;
def AVMSImm8 : Operand<i32>;
def AVMService8 : Operand<i32>;

def AVMretglue : SDNode<"AVMISD::RET_GLUE", SDTNone,
                        [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def AVMImmZExt8 : PatLeaf<(i16 imm), [{
  return isUInt<8>(N->getZExtValue());
}]>;

def AVMCompactDest : RegisterOperand<AVMCompactReg, "printCompactReg">;
def AVMCompactSource : RegisterOperand<AVMCompactReg, "printCompactReg">;
def AVMCompactCompareLeft
    : RegisterOperand<AVMCompactReg, "printCompactReg">;
def AVMCompactCompareRight
    : RegisterOperand<AVMCompactReg, "printCompactReg">;
def AVMCompactDataPointer
    : RegisterOperand<AVMCompactReg, "printCompactReg">;
def AVMColdReg : RegisterOperand<AVMColdRegClass, "printFullReg">;
def AVMStackReg : RegisterOperand<AVMGeneralReg, "printFullReg">;
def AVMProgramPair : RegisterOperand<AVMProgramPairReg, "printFullReg">;
def AVMDataAddr16 : Operand<i32>;
def AVMAbsoluteDataReg : RegisterOperand<AVMGeneralReg, "printFullReg">;
def AVMProgramLoadScalarReg : RegisterOperand<AVMGeneralReg, "printFullReg">;
def AVMProgramLoadPairReg : RegisterOperand<AVMProgramPairReg, "printFullReg">;
def AVMColdPair : RegisterOperand<AVMProgramPairReg, "printFullReg">;
def AVMColdDataPointer : RegisterOperand<AVMGeneralReg, "printFullReg">;

class AVMInst<string Asm, int Bytes, dag Operands = (ins)> : Instruction {
  let Namespace = "AVM";
  let OutOperandList = (outs);
  let InOperandList = Operands;
  let AsmString = Asm;
  let Size = Bytes;
  let isCodeGenOnly = 0;
}

class AVMCompactMoveInst<string Asm, bits<4> Family>
    : AVMInst<Asm, 1, (ins AVMCompactSource:$src)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
  bits<4> CompactFamily = Family;
}
class AVMCompactTwoAddrInst<string Asm, bits<4> Family>
    : AVMInst<Asm, 1, (ins AVMCompactDest:$src1,
                           AVMCompactSource:$src2)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
  let Constraints = "$dst = $src1";
  bits<4> CompactFamily = Family;
}
class AVMCompactCompareInst<string Asm, bits<4> Family>
    : AVMInst<Asm, 1, (ins AVMCompactCompareLeft:$left,
                           AVMCompactCompareRight:$right)> {
  let Defs = [CC];
  bits<4> CompactFamily = Family;
}
class AVMCompactLoadInst<string Asm, bits<4> Family>
    : AVMInst<Asm, 1, (ins AVMCompactDataPointer:$addr)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
  let mayLoad = 1;
  bits<4> CompactFamily = Family;
}
class AVMCompactStoreInst<string Asm, bits<4> Family>
    : AVMInst<Asm, 1, (ins AVMCompactDataPointer:$addr,
                           AVMCompactSource:$src)> {
  let mayStore = 1;
  bits<4> CompactFamily = Family;
}

// FA compact variable shifts are MC-only.  The destination is an explicit
// tied read/write operand; the count is read-only, including when both
// operands name the same compact register.
class AVMCompactVariableShiftInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDest:$src1,
                           AVMCompactSource:$count)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
  let Constraints = "$dst = $src1";
}

// FA compact immediate shifts are MC-only.  The destination is an explicit
// tied read/write operand; the immediate is an absolute unsigned four-bit
// count and has no relocation semantics.
class AVMCompactImmediateShiftInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDest:$src1, AVMImm4:$count)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
  let Constraints = "$dst = $src1";
}

class AVMF3StoreInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr,
                           AVMGeneralReg:$src)> { let mayStore = 1; }

class AVMF6PostStoreInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr_in,
                           AVMGeneralReg:$src)> {
  let OutOperandList = (outs AVMCompactDataPointer:$addr);
  let Constraints = "$addr = $addr_in";
  let mayStore = 1;
}

class AVMF5LoadInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr)> {
  let OutOperandList = (outs AVMLowRegClass:$dst);
  let mayLoad = 1;
}

class AVMF5StoreInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr,
                           AVMLowRegClass:$src)> { let mayStore = 1; }

// F7 compact-pointer postincrement memory is MC-only. The pointer is an
// explicit tied read/write operand, while load destinations are define-only.
class AVMF7LoadInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr_in)> {
  let OutOperandList = (outs AVMGeneralReg:$dst,
                             AVMCompactDataPointer:$addr);
  let Constraints = "$addr = $addr_in";
  let mayLoad = 1;
}

class AVMF7StoreInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr_in,
                           AVMGeneralReg:$src)> {
  let OutOperandList = (outs AVMCompactDataPointer:$addr);
  let Constraints = "$addr = $addr_in";
  let mayStore = 1;
}

class AVMF7PairArithmeticInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMProgramPairReg:$src1,
                           AVMProgramPairReg:$src2)> {
  let OutOperandList = (outs AVMProgramPairReg:$dst);
  let Constraints = "$dst = $src1";
}

class AVMF7PairUnaryInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMProgramPairReg:$src)> {
  let OutOperandList = (outs AVMProgramPairReg:$dst);
  let Constraints = "$dst = $src";
}

// Floating-point forms are MC-only.  The binary and unary forms are
// two-address operations; conversions use the explicit architectural operand
// order and deliberately have no condition-code operands.
class AVMMov32Inst<string Asm> : AVMInst<Asm, 2, (ins AVMProgramPair:$src)> {
  let OutOperandList = (outs AVMProgramPair:$dst);
}
class AVMFFBinaryInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMProgramPair:$src1, AVMProgramPair:$src2)> {
  let OutOperandList = (outs AVMProgramPair:$dst);
  let Constraints = "$dst = $src1";
}
class AVMFFUnaryInst<string Asm> : AVMInst<Asm, 2, (ins AVMProgramPair:$src)> {
  let OutOperandList = (outs AVMProgramPair:$dst);
  let Constraints = "$dst = $src";
}
class AVMFFPairScalarInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMGeneralReg:$scalar)> {
  let OutOperandList = (outs AVMProgramPair:$pair);
}
class AVMFFScalarPairInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMProgramPair:$pair)> {
  let OutOperandList = (outs AVMGeneralReg:$scalar);
}
class AVMFFPairPairInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMProgramPair:$src)> {
  let OutOperandList = (outs AVMProgramPair:$dst);
}
class AVMFFCompareInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMProgramPair:$left, AVMProgramPair:$right)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
}

class AVMF7BoolInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Constraints = "$dst = $src";
}

class AVMF3WideningMultiplyInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDest:$src1, AVMCompactSource:$src2)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
  let Constraints = "$dst = $src1";
}

// F1 full-register moves use the architectural PAIR48 index. This is an
// MC-only family: the encoder supplies explicit ISA register indices and no
// code-generation or scheduling metadata is attached.
class AVMFullMoveInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
}

// F5 full-register compare is MC-only. Both operands are read-only and the
// architectural condition-code register is replaced; there is no tied
// destination or implicit CC use.
class AVMFullCompareInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$left, AVMGeneralReg:$right)> {
  let Defs = [CC];
}

// F2 full-register arithmetic is MC-only.  The first input is tied to the
// explicit destination so the MC metadata records the two-address read/write
// operand without introducing any code-generation semantics.
class AVMFullArithmeticInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src1, AVMGeneralReg:$src2)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Constraints = "$dst = $src1";
}

// F9 full-register bitwise operations are MC-only. The explicit destination
// is tied to its original value, while the source is read-only. These forms
// intentionally have no implicit condition-code operands.
class AVMFullBitwiseInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src1, AVMGeneralReg:$src2)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Constraints = "$dst = $src1";
}

// FE MUL16 is MC-only. The destination is an explicit tied read/write
// operand, the source is read-only, and CC is preserved without an implicit
// use or definition.
class AVMFullMultiply16Inst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src1, AVMGeneralReg:$src2)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Constraints = "$dst = $src1";
}

// EC division and remainder are MC-only. The destination is both the original
// dividend use and the result definition, while the divisor is read-only.
// CC is preserved, so it deliberately has no implicit use or definition.
class AVMFullDivide16Inst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src1, AVMGeneralReg:$src2)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Constraints = "$dst = $src1";
}

// F1 unary and SP forms are MC-only.  The unary forms are two-address
// operations; GETSP has an explicit definition and SETSP an explicit use.
class AVMUnaryFullRegInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Constraints = "$dst = $src";
}
class AVMF4UnaryFullRegInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Constraints = "$dst = $src";
}
class AVMF4TestFullRegInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$reg)> {
    let Defs = [CC];
}
class AVMF6UnaryFullRegInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Constraints = "$dst = $src";
}
class AVMF6TestFullRegInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$reg)> {
  let Defs = [CC];
}
// F8 condition materialization is MC-only.  The destination is a complete
// define-only register, and CC is an implicit read that remains preserved.
class AVMCSetInst<string Asm> : AVMInst<Asm, 2> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Uses = [CC];
}

// FB-FD conditional moves are MC-only. The destination is a tied read/write
// operand because the false path preserves its original value; the source is
// read-only and CC is implicitly read and preserved.
class AVMConditionalMoveInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src1, AVMGeneralReg:$src2)> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Constraints = "$dst = $src1";
  let Uses = [CC];
}
class AVMF6MultiplyInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDest:$src1, AVMCompactSource:$src2)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
  let Constraints = "$dst = $src1";
}
class AVMGetSPInst<string Asm>
    : AVMInst<Asm, 2> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
  let Uses = [SP];
}
class AVMSetSPInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src)> { let Defs = [SP]; }

// Stack-register operations are MC-only and encode the explicit architectural
// register number in the low three bits of their one-byte opcode.
class AVMStackRegInst<string Asm, bits<8> Family>
    : AVMInst<Asm, 1, (ins AVMStackReg:$reg)> {
  bits<8> StackFamily = Family;
}
class AVMPush16Inst<string Asm, bits<8> Family>
    : AVMStackRegInst<Asm, Family> {
  let Uses = [SP];
  let Defs = [SP];
  let mayStore = 1;
}
class AVMPop16Inst<string Asm, bits<8> Family>
    : AVMInst<Asm, 1> {
  let OutOperandList = (outs AVMStackReg:$reg);
  let Uses = [SP];
  let Defs = [SP];
  let mayLoad = 1;
  bits<8> StackFamily = Family;
}

class AVMCompactRegU8Inst<string Asm>
    : AVMInst<Asm, 2, (ins AVMImm8:$imm)> {
  let OutOperandList = (outs AVMCompactDest:$reg);
}
class AVMCompactRegU16Inst<string Asm>
    : AVMInst<Asm, 3, (ins AVMImm16:$imm)> {
  let OutOperandList = (outs AVMCompactDest:$reg);
}
class AVMCompactRegS8TwoAddrInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDest:$src, AVMSImm8:$imm)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
  let Constraints = "$dst = $src";
}
class AVMCompactCompareIS8Inst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDest:$reg, AVMSImm8:$imm)> {
  let Defs = [CC];
}

// Cold-page operands are MC-only.  The encoder supplies an explicit ISA
// register index in the secondary byte; these definitions do not imply any
// code-generation pattern or physical-register enum layout.
class AVMColdRegImm8Inst<string Asm>
    : AVMInst<Asm, 3, (ins AVMImm8:$imm)> {
  let OutOperandList = (outs AVMColdReg:$reg);
}
class AVMColdRegImm16Inst<string Asm>
    : AVMInst<Asm, 4, (ins AVMImm16:$imm)> {
  let OutOperandList = (outs AVMColdReg:$reg);
}
class AVMColdRegS8TwoAddrInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMColdReg:$src, AVMSImm8:$imm)> {
  let OutOperandList = (outs AVMColdReg:$dst);
  let Constraints = "$dst = $src";
}
class AVMColdCompareIS8Inst<string Asm>
    : AVMInst<Asm, 3, (ins AVMColdReg:$reg, AVMSImm8:$imm)> {
  let Defs = [CC];
}
class AVMStackRegU8Inst<string Asm>
    : AVMInst<Asm, 3, (ins AVMImm8:$imm)> {
  let OutOperandList = (outs AVMStackReg:$reg);
}
class AVMStackLoadInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMImm8:$offset)> {
  let OutOperandList = (outs AVMStackReg:$reg);
  let mayLoad = 1;
  let Uses = [SP];
}
class AVMStackStoreInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMImm8:$offset, AVMStackReg:$reg)> {
  let mayStore = 1;
  let Uses = [SP];
}
class AVMCompactStackStoreInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMImm4:$offset, AVMCompactSource:$reg)> {
  let mayStore = 1;
  let Uses = [SP];
}
class AVMCompactStackLoadInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMImm4:$offset)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
  let mayLoad = 1;
  let Uses = [SP];
}

// These cold-page absolute data-space forms are MC-only.  The encoder emits
// the F0 prefix, family secondary byte, explicit ISA register index, and a
// DATA16 address field; they carry no code-generation metadata.
class AVMAbsoluteDataLoadInst<string Asm>
    : AVMInst<Asm, 4, (ins AVMDataAddr16:$addr)> {
  let OutOperandList = (outs AVMAbsoluteDataReg:$reg);
  let mayLoad = 1;
}
class AVMAbsoluteDataStoreInst<string Asm>
    : AVMInst<Asm, 4, (ins AVMDataAddr16:$addr, AVMAbsoluteDataReg:$reg)> {
  let mayStore = 1;
}

// Program-space load forms are MC-only.  Their memory operands are canonical
// program-pointer pairs parsed as a separate address register; the encoder
// supplies the PSPEC register codes explicitly.
class AVMProgramLoadScalarInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMProgramLoadPairReg:$addr)> {
  let OutOperandList = (outs AVMProgramLoadScalarReg:$dst);
  let mayLoad = 1;
}
class AVMProgramLoadPairInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMProgramLoadPairReg:$addr)> {
  let OutOperandList = (outs AVMProgramLoadPairReg:$dst);
  let mayLoad = 1;
}

// Cold 32-bit forms are MC-only.  Their RRSPEC32 byte is encoded explicitly
// from the architectural q/r mappings; it is not derived from register enums.
class AVMColdPairPairInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMColdPair:$left, AVMColdPair:$right)>;
class AVMColdPairLoadInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMColdDataPointer:$addr)> {
  let OutOperandList = (outs AVMColdPair:$dst);
  let mayLoad = 1;
}
class AVMColdPairStoreInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMColdDataPointer:$addr, AVMColdPair:$src)> {
  let mayStore = 1;
}

// General-pointer forms are MC-only.  They encode the explicit architectural
// r0-r7 indices in the dddWaaaP operand byte, independent of LLVM physical
// register enum values.
def AVMGeneralPointerDataReg
    : RegisterOperand<AVMGeneralReg, "printFullReg">;
def AVMGeneralPointerAddressReg
    : RegisterOperand<AVMGeneralReg, "printFullReg">;
class AVMGeneralPointerLoadInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMGeneralPointerAddressReg:$addr)> {
  let OutOperandList = (outs AVMGeneralPointerDataReg:$dst);
  let mayLoad = 1;
}
class AVMGeneralPointerStoreInst<string Asm>
    : AVMInst<Asm, 3,
              (ins AVMGeneralPointerAddressReg:$addr,
                   AVMGeneralPointerDataReg:$src)> { let mayStore = 1; }

// These direct forms are MC-only.  In particular, no code-generation
// patterns or scheduling state is attached to the short control instructions.
class AVMRel8ControlInst<string Asm> : AVMInst<Asm, 2, (ins AVMSImm8:$rel)>;
class AVMRel16ControlInst<string Asm> : AVMInst<Asm, 3, (ins AVMImm16:$rel)>;
class AVMConditionalRel8ControlInst<string Asm>
    : AVMRel8ControlInst<Asm> {
  let isBranch = 1;
  let isTerminator = 1;
}
class AVMConditionalRel16ControlInst<string Asm>
    : AVMRel16ControlInst<Asm> {
  let isBranch = 1;
  let isTerminator = 1;
}
class AVMRelaxableTransferPseudo<string Asm, int Bytes>
    : AVMInst<Asm, Bytes, (ins AVMImm24:$target)> {
  let isPseudo = 1;
  let hasNoSchedulingInfo = 1;
}
// This assembly-only pseudo is the implementation vehicle for .progptr; it
// emits data, not an architectural instruction.
def AVMProgPtr : Operand<i32>;
class AVMProgPtrPseudo : AVMInst<"", 3, (ins AVMProgPtr:$value)> {
  let isPseudo = 1;
  let hasNoSchedulingInfo = 1;
}
class AVMProgramPairControlInst<string Asm> : AVMInst<Asm, 1,
                                                   (ins AVMProgramPair:$pair)>;
class AVMOperandlessInst<string Asm> : AVMInst<Asm, 1>;
class AVMSImm8Inst<string Asm> : AVMInst<Asm, 2, (ins AVMSImm8:$imm)>;
class AVMService8Inst<string Asm> : AVMInst<Asm, 2, (ins AVMService8:$service)>;

class AVMCodeGenPseudo<dag Outs, dag Ins, list<dag> Pat = []>
    : Instruction {
  let Namespace = "AVM";
  let OutOperandList = Outs;
  let InOperandList = Ins;
  let AsmString = "";
  let Pattern = Pat;
  let Size = 0;
  let isPseudo = 1;
  let isCodeGenOnly = 1;
  let hasNoSchedulingInfo = 1;
}

class AVMBinary16Pseudo<SDPatternOperator Op>
    : AVMCodeGenPseudo<(outs GPR16:$dst),
                       (ins GPR16:$lhs, GPR16:$rhs),
                       [(set GPR16:$dst, (Op GPR16:$lhs, GPR16:$rhs))]> {
  let Constraints = "$dst = $lhs";
}

def COPY16_PSEUDO
    : AVMCodeGenPseudo<(outs GPR16:$dst), (ins GPR16:$src)>;
def LDI8_PSEUDO
    : AVMCodeGenPseudo<(outs GPR16:$dst), (ins i16imm:$imm),
                       [(set GPR16:$dst, AVMImmZExt8:$imm)]>;
def LDI16_PSEUDO
    : AVMCodeGenPseudo<(outs GPR16:$dst), (ins i16imm:$imm),
                       [(set GPR16:$dst, (i16 imm:$imm))]>;
def ADD16_PSEUDO : AVMBinary16Pseudo<add> { let isCommutable = 1; }
def SUB16_PSEUDO : AVMBinary16Pseudo<sub>;
def AND16_PSEUDO : AVMBinary16Pseudo<and> { let isCommutable = 1; }
def OR16_PSEUDO : AVMBinary16Pseudo<or> { let isCommutable = 1; }
def XOR16_PSEUDO : AVMBinary16Pseudo<xor> { let isCommutable = 1; }
def RET_PSEUDO : AVMCodeGenPseudo<(outs), (ins), [(AVMretglue)]> {
  let isReturn = 1;
  let isTerminator = 1;
  let isBarrier = 1;
  let Uses = [SP];
  let Defs = [SP];
  let mayLoad = 1;
}

def MOV : AVMCompactMoveInst<"mov $dst, $src", 0x0>;
def MOV_RR : AVMFullMoveInst<"mov $dst, $src">;
def CMP_RR : AVMFullCompareInst<"cmp $left, $right">;
def ADD_RR : AVMFullArithmeticInst<"add $src1, $src2">;
def SUB_RR : AVMFullArithmeticInst<"sub $src1, $src2">;
def AND_RR : AVMFullBitwiseInst<"and $src1, $src2">;
def OR_RR : AVMFullBitwiseInst<"or $src1, $src2">;
def XOR_RR : AVMFullBitwiseInst<"xor $src1, $src2">;
def MUL16 : AVMFullMultiply16Inst<"mul16 $src1, $src2">;
def UDIV16 : AVMFullDivide16Inst<"udiv16 $src1, $src2">;
def UREM16 : AVMFullDivide16Inst<"urem16 $src1, $src2">;
def SDIV16 : AVMFullDivide16Inst<"sdiv16 $src1, $src2">;
def SREM16 : AVMFullDivide16Inst<"srem16 $src1, $src2">;
def ZEXT8 : AVMUnaryFullRegInst<"zext8 $dst">;
def SWAP8 : AVMUnaryFullRegInst<"swap8 $dst">;
def GETSP : AVMGetSPInst<"getsp $dst">;
def SETSP : AVMSetSPInst<"setsp $src">;
def ADD : AVMCompactTwoAddrInst<"add $dst, $src", 0x1>;
def SUB : AVMCompactTwoAddrInst<"sub $dst, $src", 0x2>;
def CMP : AVMCompactCompareInst<"cmp $left, $right", 0x3>;
def LD8U : AVMCompactLoadInst<"ld8u $dst, [$addr]", 0x4>;
def ST8 : AVMCompactStoreInst<"st8 [$addr], $src", 0x5>;
def F3ST8 : AVMF3StoreInst<"st8 [$addr], $src">;
def F6ST8_POST : AVMF6PostStoreInst<"st8 [$addr_in+], $src">;
def F5LD8U : AVMF5LoadInst<"ld8u $dst, [$addr]">;
def F5LD16 : AVMF5LoadInst<"ld16 $dst, [$addr]">;
def F5ST16 : AVMF5StoreInst<"st16 [$addr], $src">;
def F7LD8U_POST : AVMF7LoadInst<"ld8u $dst, [$addr+]">;
def F7LD16_POST : AVMF7LoadInst<"ld16 $dst, [$addr+]">;
def F7ST16_POST : AVMF7StoreInst<"st16 [$addr+], $src">;
def ADD32 : AVMF7PairArithmeticInst<"add32 $dst, $src2">;
def SUB32 : AVMF7PairArithmeticInst<"sub32 $dst, $src2">;
def LSR32_1 : AVMF7PairUnaryInst<"lsr32.1 $dst">;
def ASR32_1 : AVMF7PairUnaryInst<"asr32.1 $dst">;
def BOOL : AVMF7BoolInst<"bool $dst">;
def MOV32 : AVMMov32Inst<"mov32 $dst, $src"> {
  let isPseudo = 1;
  let hasNoSchedulingInfo = 1;
}
def MOV32_F2 : AVMMov32Inst<"mov32 $dst, $src">;
def FADD : AVMFFBinaryInst<"fadd $dst, $src2">;
def FSUB : AVMFFBinaryInst<"fsub $dst, $src2">;
def FMUL : AVMFFBinaryInst<"fmul $dst, $src2">;
def FDIV : AVMFFBinaryInst<"fdiv $dst, $src2">;
def FMIN : AVMFFBinaryInst<"fmin $dst, $src2">;
def FMAX : AVMFFBinaryInst<"fmax $dst, $src2">;
def FNEG : AVMFFUnaryInst<"fneg $dst">;
def FABS : AVMFFUnaryInst<"fabs $dst">;
def FSQRT : AVMFFUnaryInst<"fsqrt $dst">;
def FTRUNC : AVMFFUnaryInst<"ftrunc $dst">;
def FFLOOR : AVMFFUnaryInst<"ffloor $dst">;
def FCEIL : AVMFFUnaryInst<"fceil $dst">;
def FROUND : AVMFFUnaryInst<"fround $dst">;
def S16TOF : AVMFFPairScalarInst<"s16tof $pair, $scalar">;
def U16TOF : AVMFFPairScalarInst<"u16tof $pair, $scalar">;
def FTOS16 : AVMFFScalarPairInst<"ftos16 $scalar, $pair">;
def FTOU16 : AVMFFScalarPairInst<"ftou16 $scalar, $pair">;
def S32TOF : AVMFFPairPairInst<"s32tof $dst, $src">;
def U32TOF : AVMFFPairPairInst<"u32tof $dst, $src">;
def FTOS32 : AVMFFPairPairInst<"ftos32 $dst, $src">;
def FTOU32 : AVMFFPairPairInst<"ftou32 $dst, $src">;
def FCMP : AVMFFCompareInst<"fcmp $dst, $left, $right">;
def FCLASS : AVMFFScalarPairInst<"fclass $scalar, $pair">;
def MULU8W : AVMF3WideningMultiplyInst<"mulu8.w $dst, $src2">;
def MULS8W : AVMF3WideningMultiplyInst<"muls8.w $dst, $src2">;
def MULSU8W : AVMF3WideningMultiplyInst<"mulsu8.w $dst, $src2">;
def LD16 : AVMCompactLoadInst<"ld16 $dst, [$addr]", 0x6>;
def ST16 : AVMCompactStoreInst<"st16 [$addr], $src", 0x7>;
def AND : AVMCompactTwoAddrInst<"and $dst, $src", 0x8>;
def OR : AVMCompactTwoAddrInst<"or $dst, $src", 0x9>;
def XOR : AVMCompactTwoAddrInst<"xor $dst, $src", 0xa>;

def PUSH16 : AVMPush16Inst<"push16 $reg", 0xb0>;
def POP16 : AVMPop16Inst<"pop16 $reg", 0xb8>;

def LDI8 : AVMCompactRegU8Inst<"ldi8 $reg, $imm">;
def LDI16 : AVMCompactRegU16Inst<"ldi16 $reg, $imm">;
def ADDIS8 : AVMCompactRegS8TwoAddrInst<"addi.s8 $dst, $imm">;
def CMPIS8 : AVMCompactCompareIS8Inst<"cmpi.s8 $reg, $imm">;

def COLDLDI8 : AVMColdRegImm8Inst<"ldi8 $reg, $imm">;
def COLDLDI16 : AVMColdRegImm16Inst<"ldi16 $reg, $imm">;
def COLDADDIS8 : AVMColdRegS8TwoAddrInst<"addi.s8 $dst, $imm">;
def COLDCMPIS8 : AVMColdCompareIS8Inst<"cmpi.s8 $reg, $imm">;
def LEASP : AVMStackRegU8Inst<"leasp $reg, $imm"> { let Uses = [SP]; }
def LDSP8U : AVMStackLoadInst<"ldsp8u $reg, [sp+$offset]">;
def LDSP8U_COMPACT : AVMCompactStackLoadInst<"ldsp8u $dst, [sp+$offset]">;
def LDSP8S : AVMStackLoadInst<"ldsp8s $reg, [sp+$offset]">;
def STSP8 : AVMStackStoreInst<"stsp8 [sp+$offset], $reg">;
def STSP8_COMPACT : AVMCompactStackStoreInst<"stsp8 [sp+$offset], $reg">;
def LDSP16 : AVMStackLoadInst<"ldsp16 $reg, [sp+$offset]">;
def STSP16 : AVMStackStoreInst<"stsp16 [sp+$offset], $reg">;
def LDSP16_COMPACT : AVMCompactStackLoadInst<"ldsp16 $dst, [sp+$offset]">;
def STSP16_COMPACT : AVMCompactStackStoreInst<"stsp16 [sp+$offset], $reg">;
def LSL16_1 : AVMF4UnaryFullRegInst<"lsl16.1 $dst">;
def LSR16_1 : AVMF4UnaryFullRegInst<"lsr16.1 $dst">;
def ASR16_1 : AVMF4UnaryFullRegInst<"asr16.1 $dst">;
def NOT16 : AVMF4UnaryFullRegInst<"not16 $dst">;
def TST8 : AVMF4TestFullRegInst<"tst8 $reg">;
def INC16 : AVMF4UnaryFullRegInst<"inc16 $dst">;
def DEC16 : AVMF4UnaryFullRegInst<"dec16 $dst">;
def BSWAP16 : AVMF6UnaryFullRegInst<"bswap16 $dst">;
def TST16 : AVMF6TestFullRegInst<"tst16 $reg">;
def MUL8 : AVMF6MultiplyInst<"mul8 $dst, $src2">;
def SEXT8 : AVMF6UnaryFullRegInst<"sext8 $dst">;
def NEG16 : AVMF6UnaryFullRegInst<"neg16 $dst">;

def CSET_EQ : AVMCSetInst<"cset.eq $dst">;
def CSET_NE : AVMCSetInst<"cset.ne $dst">;
def CSET_ULT : AVMCSetInst<"cset.ult $dst">;
def CSET_UGE : AVMCSetInst<"cset.uge $dst">;
def CSET_SLT : AVMCSetInst<"cset.slt $dst">;
def CSET_SGE : AVMCSetInst<"cset.sge $dst">;

def CMOV_EQ : AVMConditionalMoveInst<"cmov.eq $dst, $src2">;
def CMOV_NE : AVMConditionalMoveInst<"cmov.ne $dst, $src2">;
def CMOV_ULT : AVMConditionalMoveInst<"cmov.ult $dst, $src2">;
def CMOV_UGE : AVMConditionalMoveInst<"cmov.uge $dst, $src2">;
def CMOV_SLT : AVMConditionalMoveInst<"cmov.slt $dst, $src2">;
def CMOV_SGE : AVMConditionalMoveInst<"cmov.sge $dst, $src2">;

def SHL16V : AVMCompactVariableShiftInst<"shl16v $dst, $count">;
def LSR16V : AVMCompactVariableShiftInst<"lsr16v $dst, $count">;
def ASR16V : AVMCompactVariableShiftInst<"asr16v $dst, $count">;
def LSL16I : AVMCompactImmediateShiftInst<"lsl16i $dst, $count">;
def LSR16I : AVMCompactImmediateShiftInst<"lsr16i $dst, $count">;
def ASR16I : AVMCompactImmediateShiftInst<"asr16i $dst, $count">;

def LDM8U : AVMAbsoluteDataLoadInst<"ldm8u $reg, [$addr]">;
def STM8 : AVMAbsoluteDataStoreInst<"stm8 [$addr], $reg">;
def LDM16 : AVMAbsoluteDataLoadInst<"ldm16 $reg, [$addr]">;
def STM16 : AVMAbsoluteDataStoreInst<"stm16 [$addr], $reg">;

def LDP8U : AVMProgramLoadScalarInst<"ldp8u $dst, [$addr]">;
def LDP8S : AVMProgramLoadScalarInst<"ldp8s $dst, [$addr]">;
def LDP16 : AVMProgramLoadScalarInst<"ldp16 $dst, [$addr]">;
def LDP24 : AVMProgramLoadPairInst<"ldp24 $dst, [$addr]">;
def LDP32 : AVMProgramLoadPairInst<"ldp32 $dst, [$addr]">;
def LDP8U_POST : AVMProgramLoadScalarInst<"ldp8u $dst, [$addr+]">;
def LDP16_POST : AVMProgramLoadScalarInst<"ldp16 $dst, [$addr+]">;
def LDP24_POST : AVMProgramLoadPairInst<"ldp24 $dst, [$addr+]">;
def LDP32_POST : AVMProgramLoadPairInst<"ldp32 $dst, [$addr+]">;

def CMP32 : AVMColdPairPairInst<"cmp32 $left, $right"> {
  let Defs = [CC];
}
def LD32 : AVMColdPairLoadInst<"ld32 $dst, [$addr]">;
def ST32 : AVMColdPairStoreInst<"st32 [$addr], $src">;

def GPLD8U : AVMGeneralPointerLoadInst<"ld8u $dst, [$addr]">;
def GPLD16 : AVMGeneralPointerLoadInst<"ld16 $dst, [$addr]">;
def GPLD8U_POST : AVMGeneralPointerLoadInst<"ld8u $dst, [$addr+]">;
def GPLD16_POST : AVMGeneralPointerLoadInst<"ld16 $dst, [$addr+]">;
def GPST8 : AVMGeneralPointerStoreInst<"st8 [$addr], $src">;
def GPST16 : AVMGeneralPointerStoreInst<"st16 [$addr], $src">;
def GPST8_POST : AVMGeneralPointerStoreInst<"st8 [$addr+], $src">;
def GPST16_POST : AVMGeneralPointerStoreInst<"st16 [$addr+], $src">;

def BREQ8 : AVMConditionalRel8ControlInst<"breq8 $rel"> { let Uses = [CC]; }
def BRNE8 : AVMConditionalRel8ControlInst<"brne8 $rel"> { let Uses = [CC]; }
def BRULT8 : AVMConditionalRel8ControlInst<"brult8 $rel"> { let Uses = [CC]; }
def BRSLT8 : AVMConditionalRel8ControlInst<"brslt8 $rel"> { let Uses = [CC]; }
def BRUGE8 : AVMConditionalRel8ControlInst<"bruge8 $rel"> { let Uses = [CC]; }
def BRSGE8 : AVMConditionalRel8ControlInst<"brsge8 $rel"> { let Uses = [CC]; }
def BREQ16 : AVMConditionalRel16ControlInst<"breq16 $rel"> { let Uses = [CC]; }
def BRNE16 : AVMConditionalRel16ControlInst<"brne16 $rel"> { let Uses = [CC]; }
def BRULT16 : AVMConditionalRel16ControlInst<"brult16 $rel"> { let Uses = [CC]; }
def BRUGE16 : AVMConditionalRel16ControlInst<"bruge16 $rel"> { let Uses = [CC]; }
def BRSLT16 : AVMConditionalRel16ControlInst<"brslt16 $rel"> { let Uses = [CC]; }
def BRSGE16 : AVMConditionalRel16ControlInst<"brsge16 $rel"> { let Uses = [CC]; }
def JMP8 : AVMRel8ControlInst<"jmp8 $rel"> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def CALL8 : AVMRel8ControlInst<"call8 $rel"> {
  let isCall = 1;
  let Uses = [SP];
  let Defs = [SP];
  let mayStore = 1;
}
def ADJSP : AVMSImm8Inst<"adjsp $imm"> { let Defs = [SP]; let Uses = [SP]; }
def SYS : AVMService8Inst<"sys $service"> { let hasSideEffects = 1; }

def JMPF : AVMInst<"jmpf $target", 4, (ins AVMImm24:$target)> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

def CALLF : AVMInst<"callf $target", 4, (ins AVMImm24:$target)> {
  let isCall = 1;
  let Uses = [SP];
  let Defs = [SP];
  let mayStore = 1;
}

def JMP16 : AVMRel16ControlInst<"jmp16 $rel"> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def CALL16 : AVMRel16ControlInst<"call16 $rel"> {
  let isCall = 1;
  let Uses = [SP];
  let Defs = [SP];
  let mayStore = 1;
}
def JMPP : AVMProgramPairControlInst<"jmpp $pair"> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def CALLP : AVMProgramPairControlInst<"callp $pair"> {
  let isCall = 1;
  let Uses = [SP];
  let Defs = [SP];
  let mayStore = 1;
}
def RET : AVMOperandlessInst<"ret"> {
  let isReturn = 1;
  let isTerminator = 1;
  let isBarrier = 1;
  let Uses = [SP];
  let Defs = [SP];
  let mayLoad = 1;
}
def PROGPTR : AVMProgPtrPseudo;

// These are assembly-only pseudos.  The MC code emitter expands each to its
// maximal encoding and emits the R_AVM_RELAX marker alongside R_AVM_FAR24.
def RELAX_JMP : AVMRelaxableTransferPseudo<"jmp $target", 4> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def RELAX_CALL : AVMRelaxableTransferPseudo<"call $target", 4> {
  let isCall = 1;
  let Uses = [SP];
  let Defs = [SP];
  let mayStore = 1;
}
def RELAX_BR_EQ : AVMRelaxableTransferPseudo<"breq $target", 6> {
  let Uses = [CC];
  let isBranch = 1;
  let isTerminator = 1;
}
def RELAX_BR_NE : AVMRelaxableTransferPseudo<"brne $target", 6> {
  let Uses = [CC];
  let isBranch = 1;
  let isTerminator = 1;
}
def RELAX_BR_ULT : AVMRelaxableTransferPseudo<"brult $target", 6> {
  let Uses = [CC];
  let isBranch = 1;
  let isTerminator = 1;
}
def RELAX_BR_UGE : AVMRelaxableTransferPseudo<"bruge $target", 6> {
  let Uses = [CC];
  let isBranch = 1;
  let isTerminator = 1;
}
def RELAX_BR_SLT : AVMRelaxableTransferPseudo<"brslt $target", 6> {
  let Uses = [CC];
  let isBranch = 1;
  let isTerminator = 1;
}
def RELAX_BR_SGE : AVMRelaxableTransferPseudo<"brsge $target", 6> {
  let Uses = [CC];
  let isBranch = 1;
  let isTerminator = 1;
}
