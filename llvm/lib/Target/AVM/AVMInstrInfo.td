//===-- AVMInstrInfo.td - AVM MC instruction definitions ---*- tablegen -*-===//

// The current opcode map is introduced in a later change.  These are the
// already-verified, direct far transfers whose encodings match docs/arch.md.
def AVMImm24 : Operand<i32>;

class AVMInst<string Asm, int Bytes, dag Operands = (ins)> : Instruction {
  let Namespace = "AVM";
  let OutOperandList = (outs);
  let InOperandList = Operands;
  let AsmString = Asm;
  let Size = Bytes;
  let isCodeGenOnly = 0;
}

def JMPF : AVMInst<"jmpf $target", 4, (ins AVMImm24:$target)> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

def CALLF : AVMInst<"callf $target", 4, (ins AVMImm24:$target)> {
  let isCall = 1;
}
