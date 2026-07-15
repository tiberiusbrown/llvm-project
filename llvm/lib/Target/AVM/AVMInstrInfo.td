//===-- AVMInstrInfo.td - AVM MC instruction definitions ---*- tablegen -*-===//

// The current opcode map is introduced in a later change.  These are the
// already-verified, direct far transfers whose encodings match docs/arch.md.
def AVMImm24 : Operand<i32>;
def AVMImm8 : Operand<i32>;
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
def AVMStackReg : RegisterOperand<AVMGeneralReg, "printFullReg">;
def AVMProgramPair : RegisterOperand<AVMProgramPairReg, "printFullReg">;

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
