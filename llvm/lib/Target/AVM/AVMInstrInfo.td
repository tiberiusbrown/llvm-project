//===-- AVMInstrInfo.td - AVM MC instruction definitions ---*- tablegen -*-===//

// The current opcode map is introduced in a later change.  These are the
// already-verified, direct far transfers whose encodings match docs/arch.md.
def AVMImm24 : Operand<i32>;

def AVMCompactDest : RegisterOperand<AVMCompactReg, "printCompactReg">;
def AVMCompactSource : RegisterOperand<AVMCompactReg, "printCompactReg">;
def AVMCompactCompareLeft
    : RegisterOperand<AVMCompactReg, "printCompactReg">;
def AVMCompactCompareRight
    : RegisterOperand<AVMCompactReg, "printCompactReg">;
def AVMCompactDataPointer
    : RegisterOperand<AVMCompactReg, "printCompactReg">;

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

def JMPF : AVMInst<"jmpf $target", 4, (ins AVMImm24:$target)> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

def CALLF : AVMInst<"callf $target", 4, (ins AVMImm24:$target)> {
  let isCall = 1;
}
