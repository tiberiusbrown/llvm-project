//===-- AVMInstrInfo.td - AVM MC instruction definitions ---*- tablegen -*-===//

// The current opcode map is introduced in a later change.  These are the
// already-verified, direct far transfers whose encodings match docs/arch.md.
def AVMImm24 : Operand<i32>;
def AVMImm8 : Operand<i32>;
def AVMImm4 : Operand<i32>;
def AVMImm16 : Operand<i32>;
def AVMSImm8 : Operand<i32>;
def AVMService8 : Operand<i32>;

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

// The one-byte compact matrix is deliberately MC-only. The high nibble is
// fixed per family; the encoder supplies the two compact-register indices in
// the low nibble. No selection pattern or scheduling metadata belongs here.
class AVMCompactMatrixInst<string Asm, bits<4> Family, dag Operands>
    : AVMInst<Asm, 1, Operands> {
  bits<4> CompactFamily = Family;
}

class AVMF3StoreInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr,
                           AVMGeneralReg:$src)>;

class AVMF6PostStoreInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr_in,
                           AVMGeneralReg:$src)> {
  let OutOperandList = (outs AVMCompactDataPointer:$addr);
  let Constraints = "$addr = $addr_in";
}

class AVMF5LoadInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr)> {
  let OutOperandList = (outs AVMLowRegClass:$dst);
}

class AVMF5StoreInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr,
                           AVMLowRegClass:$src)>;

// F7 compact-pointer postincrement memory is MC-only. The pointer is an
// explicit tied read/write operand, while load destinations are define-only.
class AVMF7LoadInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr_in)> {
  let OutOperandList = (outs AVMGeneralReg:$dst,
                             AVMCompactDataPointer:$addr);
  let Constraints = "$addr = $addr_in";
}

class AVMF7StoreInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDataPointer:$addr_in,
                           AVMGeneralReg:$src)> {
  let OutOperandList = (outs AVMCompactDataPointer:$addr);
  let Constraints = "$addr = $addr_in";
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
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$dst, AVMGeneralReg:$src)>;

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
class AVMF6MultiplyInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDest:$src1, AVMCompactSource:$src2)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
  let Constraints = "$dst = $src1";
}
class AVMGetSPInst<string Asm>
    : AVMInst<Asm, 2> {
  let OutOperandList = (outs AVMGeneralReg:$dst);
}
class AVMSetSPInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMGeneralReg:$src)>;

// Stack-register operations are MC-only and encode the explicit architectural
// register number in the low three bits of their one-byte opcode.
class AVMStackRegInst<string Asm, bits<8> Family>
    : AVMInst<Asm, 1, (ins AVMStackReg:$reg)> {
  bits<8> StackFamily = Family;
}

class AVMCompactRegU8Inst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDest:$reg, AVMImm8:$imm)>;
class AVMCompactRegU16Inst<string Asm>
    : AVMInst<Asm, 3, (ins AVMCompactDest:$reg, AVMImm16:$imm)>;
class AVMCompactRegS8Inst<string Asm>
    : AVMInst<Asm, 2, (ins AVMCompactDest:$reg, AVMSImm8:$imm)>;

// Cold-page operands are MC-only.  The encoder supplies an explicit ISA
// register index in the secondary byte; these definitions do not imply any
// code-generation pattern or physical-register enum layout.
class AVMColdRegImm8Inst<string Asm>
    : AVMInst<Asm, 3, (ins AVMColdReg:$reg, AVMImm8:$imm)>;
class AVMColdRegImm16Inst<string Asm>
    : AVMInst<Asm, 4, (ins AVMColdReg:$reg, AVMImm16:$imm)>;
class AVMColdRegS8Inst<string Asm>
    : AVMInst<Asm, 3, (ins AVMColdReg:$reg, AVMSImm8:$imm)>;
class AVMStackRegU8Inst<string Asm>
    : AVMInst<Asm, 3, (ins AVMStackReg:$reg, AVMImm8:$imm)>;
class AVMStackLoadInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMStackReg:$reg, AVMImm8:$offset)>;
class AVMStackStoreInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMImm8:$offset, AVMStackReg:$reg)>;
class AVMCompactStackStoreInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMImm4:$offset, AVMCompactSource:$reg)>;
class AVMCompactStackLoadInst<string Asm>
    : AVMInst<Asm, 2, (ins AVMImm4:$offset)> {
  let OutOperandList = (outs AVMCompactDest:$dst);
}

// These cold-page absolute data-space forms are MC-only.  The encoder emits
// the F0 prefix, family secondary byte, explicit ISA register index, and a
// DATA16 address field; they carry no code-generation metadata.
class AVMAbsoluteDataLoadInst<string Asm>
    : AVMInst<Asm, 4, (ins AVMAbsoluteDataReg:$reg, AVMDataAddr16:$addr)>;
class AVMAbsoluteDataStoreInst<string Asm>
    : AVMInst<Asm, 4, (ins AVMDataAddr16:$addr, AVMAbsoluteDataReg:$reg)>;

// Program-space load forms are MC-only.  Their memory operands are canonical
// program-pointer pairs parsed as a separate address register; the encoder
// supplies the PSPEC register codes explicitly.
class AVMProgramLoadScalarInst<string Asm>
    : AVMInst<Asm, 3,
              (ins AVMProgramLoadScalarReg:$dst, AVMProgramLoadPairReg:$addr)>;
class AVMProgramLoadPairInst<string Asm>
    : AVMInst<Asm, 3,
              (ins AVMProgramLoadPairReg:$dst, AVMProgramLoadPairReg:$addr)>;

// Cold 32-bit forms are MC-only.  Their RRSPEC32 byte is encoded explicitly
// from the architectural q/r mappings; it is not derived from register enums.
class AVMColdPairPairInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMColdPair:$left, AVMColdPair:$right)>;
class AVMColdPairLoadInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMColdPair:$dst, AVMColdDataPointer:$addr)>;
class AVMColdPairStoreInst<string Asm>
    : AVMInst<Asm, 3, (ins AVMColdDataPointer:$addr, AVMColdPair:$src)>;

// General-pointer forms are MC-only.  They encode the explicit architectural
// r0-r7 indices in the dddWaaaP operand byte, independent of LLVM physical
// register enum values.
def AVMGeneralPointerDataReg
    : RegisterOperand<AVMGeneralReg, "printFullReg">;
def AVMGeneralPointerAddressReg
    : RegisterOperand<AVMGeneralReg, "printFullReg">;
class AVMGeneralPointerLoadInst<string Asm>
    : AVMInst<Asm, 3,
              (ins AVMGeneralPointerDataReg:$dst,
                   AVMGeneralPointerAddressReg:$addr)>;
class AVMGeneralPointerStoreInst<string Asm>
    : AVMInst<Asm, 3,
              (ins AVMGeneralPointerAddressReg:$addr,
                   AVMGeneralPointerDataReg:$src)>;

// These direct forms are MC-only.  In particular, no code-generation
// patterns or scheduling state is attached to the short control instructions.
class AVMRel8ControlInst<string Asm> : AVMInst<Asm, 2, (ins AVMSImm8:$rel)>;
class AVMRel16ControlInst<string Asm> : AVMInst<Asm, 3, (ins AVMImm16:$rel)>;
class AVMProgramPairControlInst<string Asm> : AVMInst<Asm, 1,
                                                   (ins AVMProgramPair:$pair)>;
class AVMOperandlessInst<string Asm> : AVMInst<Asm, 1>;
class AVMSImm8Inst<string Asm> : AVMInst<Asm, 2, (ins AVMSImm8:$imm)>;
class AVMService8Inst<string Asm> : AVMInst<Asm, 2, (ins AVMService8:$service)>;

def MOV : AVMCompactMatrixInst<"mov $dst, $src", 0x0,
    (ins AVMCompactDest:$dst, AVMCompactSource:$src)>;
def MOV_RR : AVMFullMoveInst<"mov $dst, $src">;
def CMP_RR : AVMFullCompareInst<"cmp $left, $right">;
def ADD_RR : AVMFullArithmeticInst<"add $src1, $src2">;
def SUB_RR : AVMFullArithmeticInst<"sub $src1, $src2">;
def ZEXT8 : AVMUnaryFullRegInst<"zext8 $dst">;
def SWAP8 : AVMUnaryFullRegInst<"swap8 $dst">;
def GETSP : AVMGetSPInst<"getsp $dst">;
def SETSP : AVMSetSPInst<"setsp $src">;
def ADD : AVMCompactMatrixInst<"add $dst, $src", 0x1,
    (ins AVMCompactDest:$dst, AVMCompactSource:$src)>;
def SUB : AVMCompactMatrixInst<"sub $dst, $src", 0x2,
    (ins AVMCompactDest:$dst, AVMCompactSource:$src)>;
def CMP : AVMCompactMatrixInst<"cmp $left, $right", 0x3,
    (ins AVMCompactCompareLeft:$left, AVMCompactCompareRight:$right)> {
  let Defs = [CC];
}
def LD8U : AVMCompactMatrixInst<"ld8u $dst, [$addr]", 0x4,
    (ins AVMCompactDest:$dst, AVMCompactDataPointer:$addr)>;
def ST8 : AVMCompactMatrixInst<"st8 [$addr], $src", 0x5,
    (ins AVMCompactDataPointer:$addr, AVMCompactSource:$src)>;
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
def MULU8W : AVMF3WideningMultiplyInst<"mulu8.w $dst, $src2">;
def MULS8W : AVMF3WideningMultiplyInst<"muls8.w $dst, $src2">;
def MULSU8W : AVMF3WideningMultiplyInst<"mulsu8.w $dst, $src2">;
def LD16 : AVMCompactMatrixInst<"ld16 $dst, [$addr]", 0x6,
    (ins AVMCompactDest:$dst, AVMCompactDataPointer:$addr)>;
def ST16 : AVMCompactMatrixInst<"st16 [$addr], $src", 0x7,
    (ins AVMCompactDataPointer:$addr, AVMCompactSource:$src)>;
def AND : AVMCompactMatrixInst<"and $dst, $src", 0x8,
    (ins AVMCompactDest:$dst, AVMCompactSource:$src)>;
def OR : AVMCompactMatrixInst<"or $dst, $src", 0x9,
    (ins AVMCompactDest:$dst, AVMCompactSource:$src)>;
def XOR : AVMCompactMatrixInst<"xor $dst, $src", 0xa,
    (ins AVMCompactDest:$dst, AVMCompactSource:$src)>;

def PUSH16 : AVMStackRegInst<"push16 $reg", 0xb0>;
def POP16 : AVMStackRegInst<"pop16 $reg", 0xb8>;

def LDI8 : AVMCompactRegU8Inst<"ldi8 $reg, $imm">;
def LDI16 : AVMCompactRegU16Inst<"ldi16 $reg, $imm">;
def ADDIS8 : AVMCompactRegS8Inst<"addi.s8 $reg, $imm">;
def CMPIS8 : AVMCompactRegS8Inst<"cmpi.s8 $reg, $imm"> {
  let Defs = [CC];
}

def COLDLDI8 : AVMColdRegImm8Inst<"ldi8 $reg, $imm">;
def COLDLDI16 : AVMColdRegImm16Inst<"ldi16 $reg, $imm">;
def COLDADDIS8 : AVMColdRegS8Inst<"addi.s8 $reg, $imm">;
def COLDCMPIS8 : AVMColdRegS8Inst<"cmpi.s8 $reg, $imm"> {
  let Defs = [CC];
}
def LEASP : AVMStackRegU8Inst<"leasp $reg, $imm">;
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

def BREQ : AVMRel8ControlInst<"breq $rel"> { let Uses = [CC]; }
def BRNE : AVMRel8ControlInst<"brne $rel"> { let Uses = [CC]; }
def BRULT : AVMRel8ControlInst<"brult $rel"> { let Uses = [CC]; }
def BRSLT : AVMRel8ControlInst<"brslt $rel"> { let Uses = [CC]; }
def JMP : AVMRel8ControlInst<"jmp $rel"> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def CALL : AVMRel8ControlInst<"call $rel"> { let isCall = 1; }
def ADJSP : AVMSImm8Inst<"adjsp $imm">;
def SYS : AVMService8Inst<"sys $service">;

def JMPF : AVMInst<"jmpf $target", 4, (ins AVMImm24:$target)> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

def CALLF : AVMInst<"callf $target", 4, (ins AVMImm24:$target)> {
  let isCall = 1;
}

def JMP16 : AVMRel16ControlInst<"jmp16 $rel"> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def CALL16 : AVMRel16ControlInst<"call16 $rel"> { let isCall = 1; }
def JMPP : AVMProgramPairControlInst<"jmpp $pair"> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def CALLP : AVMProgramPairControlInst<"callp $pair"> { let isCall = 1; }
def RET : AVMOperandlessInst<"ret"> {
  let isReturn = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
