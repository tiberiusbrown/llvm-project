//===-- ABCStubs.td - Minimal ABC register/instruction records -*- tablegen -*-===//

let Namespace = "ABC" in {
def STK : Register<"stk"> {
  let HWEncoding = 0;
}

def STACK : RegisterClass<"ABC", [i8, i16, i32], 8, (add STK)>;
}

def imm8 : Operand<i8>;
def imm16 : Operand<i16>;
def imm24 : Operand<i32>;
def imm32 : Operand<i32>;

class ABCInst<dag outs, dag ins, string asm, list<dag> pattern = []>
    : Instruction {
  let Namespace = "ABC";
  let DecoderNamespace = "ABC";
  let Size = 1;
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asm;
  let Pattern = pattern;
  let hasSideEffects = 1;
}

class ABCOp0<string asm> : ABCInst<(outs), (ins), asm>;
class ABCOp8<string asm> : ABCInst<(outs), (ins imm8:$imm0),
                                    !strconcat(asm, "\t$imm0")>;
class ABCOp16<string asm> : ABCInst<(outs), (ins imm16:$imm0),
                                     !strconcat(asm, "\t$imm0")>;
class ABCOp24<string asm> : ABCInst<(outs), (ins imm24:$imm0),
                                     !strconcat(asm, "\t$imm0")>;
class ABCOp32<string asm> : ABCInst<(outs), (ins imm32:$imm0),
                                     !strconcat(asm, "\t$imm0")>;
class ABCOp8x8<string asm> : ABCInst<(outs), (ins imm8:$imm0, imm8:$imm1),
                                      !strconcat(asm, "\t$imm0, $imm1")>;
class ABCOp8x16<string asm> : ABCInst<(outs), (ins imm8:$imm0, imm16:$imm1),
                                       !strconcat(asm, "\t$imm0, $imm1")>;
class ABCOp16x16<string asm> : ABCInst<(outs), (ins imm16:$imm0, imm16:$imm1),
                                        !strconcat(asm, "\t$imm0, $imm1")>;
class ABCOp16x24<string asm> : ABCInst<(outs), (ins imm16:$imm0, imm24:$imm1),
                                        !strconcat(asm, "\t$imm0, $imm1")>;

def NOP : ABCOp0<"nop">;
def PUSH : ABCOp8<"push">;
def P0 : ABCOp0<"p0">;
def P1 : ABCOp0<"p1">;
def P2 : ABCOp0<"p2">;
def P3 : ABCOp0<"p3">;
def P4 : ABCOp0<"p4">;
def P5 : ABCOp0<"p5">;
def P6 : ABCOp0<"p6">;
def P7 : ABCOp0<"p7">;
def P8 : ABCOp0<"p8">;
def P16 : ABCOp0<"p16">;
def P32 : ABCOp0<"p32">;
def P64 : ABCOp0<"p64">;
def P128 : ABCOp0<"p128">;
def P00 : ABCOp0<"p00">;
def P000 : ABCOp0<"p000">;
def P0000 : ABCOp0<"p0000">;
def PZ8 : ABCOp0<"pz8">;
def PZ16 : ABCOp0<"pz16">;
def PUSHG : ABCOp16<"pushg">;
// Integer-only pseudo forms.  They use the same bytecode as PUSHG/PUSHL,
// but must never be treated as global/program relocations.
def PUSH2 : ABCOp16<"push2">;
def PUSHL : ABCOp24<"pushl">;
def PUSH3 : ABCOp24<"push3">;
def PUSH4 : ABCOp32<"push4">;
def SEXT : ABCOp0<"sext">;
def SEXT2 : ABCOp0<"sext2">;
def SEXT3 : ABCOp0<"sext3">;
def DUP : ABCOp0<"dup">;
def DUP2 : ABCOp0<"dup2">;
def DUP3 : ABCOp0<"dup3">;
def DUP4 : ABCOp0<"dup4">;
def DUP5 : ABCOp0<"dup5">;
def DUP6 : ABCOp0<"dup6">;
def DUP7 : ABCOp0<"dup7">;
def DUP8 : ABCOp0<"dup8">;
def DUPW : ABCOp0<"dupw">;
def DUPW2 : ABCOp0<"dupw2">;
def DUPW3 : ABCOp0<"dupw3">;
def DUPW4 : ABCOp0<"dupw4">;
def DUPW5 : ABCOp0<"dupw5">;
def DUPW6 : ABCOp0<"dupw6">;
def DUPW7 : ABCOp0<"dupw7">;
def DUPW8 : ABCOp0<"dupw8">;
def GETL : ABCOp8<"getl">;
def GETL2 : ABCOp8<"getl2">;
def GETL4 : ABCOp8<"getl4">;
def GETLN : ABCOp8x8<"getln">;
def SETL : ABCOp8<"setl">;
def SETL2 : ABCOp8<"setl2">;
def SETL4 : ABCOp8<"setl4">;
def SETLN : ABCOp8x8<"setln">;
def GETG : ABCOp16<"getg">;
def GETG2 : ABCOp16<"getg2">;
def GETG4 : ABCOp16<"getg4">;
def GETGN : ABCOp8x16<"getgn">;
def GTGB : ABCOp8<"gtgb">;
def GTGB2 : ABCOp8<"gtgb2">;
def GTGB4 : ABCOp8<"gtgb4">;
def SETG : ABCOp16<"setg">;
def SETG2 : ABCOp16<"setg2">;
def SETG4 : ABCOp16<"setg4">;
def SETGN : ABCOp8x16<"setgn">;
def GETP : ABCOp0<"getp">;
def GETPN : ABCOp8<"getpn">;
def GETR : ABCOp0<"getr">;
def GETR2 : ABCOp0<"getr2">;
def GETRN : ABCOp8<"getrn">;
def SETR : ABCOp0<"setr">;
def SETR2 : ABCOp0<"setr2">;
def SETRN : ABCOp8<"setrn">;
def POP : ABCOp0<"pop">;
def POP2 : ABCOp0<"pop2">;
def POP3 : ABCOp0<"pop3">;
def POP4 : ABCOp0<"pop4">;
def POPN : ABCOp8<"popn">;
def ALLOC : ABCOp8<"alloc">;
def AIXB1 : ABCOp8<"aixb1">;
def AIDXB : ABCOp8x8<"aidxb">;
def AIDX : ABCOp16x16<"aidx">;
def PIDXB : ABCOp8x8<"pidxb">;
def PIDX : ABCOp16x24<"pidx">;
def UAIDX : ABCOp16<"uaidx">;
def UPIDX : ABCOp16<"upidx">;
def REFL : ABCOp8<"refl">;
def REFGB : ABCOp8<"refgb">;
def INC : ABCOp0<"inc">;
def DEC : ABCOp0<"dec">;
def LINC : ABCOp8<"linc">;
def PINC : ABCOp0<"pinc">;
def PINC2 : ABCOp0<"pinc2">;
def PINC3 : ABCOp0<"pinc3">;
def PINC4 : ABCOp0<"pinc4">;
def PDEC : ABCOp0<"pdec">;
def PDEC2 : ABCOp0<"pdec2">;
def PDEC3 : ABCOp0<"pdec3">;
def PDEC4 : ABCOp0<"pdec4">;
def PINCF : ABCOp0<"pincf">;
def PDECF : ABCOp0<"pdecf">;
def ADD : ABCOp0<"add">;
def ADD2 : ABCOp0<"add2">;
def ADD3 : ABCOp0<"add3">;
def ADD4 : ABCOp0<"add4">;
def SUB : ABCOp0<"sub">;
def SUB2 : ABCOp0<"sub2">;
def SUB3 : ABCOp0<"sub3">;
def SUB4 : ABCOp0<"sub4">;
def ADD2B : ABCOp0<"add2b">;
def ADD3B : ABCOp0<"add3b">;
def SUB2B : ABCOp0<"sub2b">;
def MUL2B : ABCOp0<"mul2b">;
def MUL : ABCOp0<"mul">;
def MUL2 : ABCOp0<"mul2">;
def MUL3 : ABCOp0<"mul3">;
def MUL4 : ABCOp0<"mul4">;
def UDIV2 : ABCOp0<"udiv2">;
def UDIV4 : ABCOp0<"udiv4">;
def DIV2 : ABCOp0<"div2">;
def DIV4 : ABCOp0<"div4">;
def UMOD2 : ABCOp0<"umod2">;
def UMOD4 : ABCOp0<"umod4">;
def MOD2 : ABCOp0<"mod2">;
def MOD4 : ABCOp0<"mod4">;
def LSL : ABCOp0<"lsl">;
def LSL2 : ABCOp0<"lsl2">;
def LSL4 : ABCOp0<"lsl4">;
def LSR : ABCOp0<"lsr">;
def LSR2 : ABCOp0<"lsr2">;
def LSR4 : ABCOp0<"lsr4">;
def ASR : ABCOp0<"asr">;
def ASR2 : ABCOp0<"asr2">;
def ASR4 : ABCOp0<"asr4">;
def AND : ABCOp0<"and">;
def AND2 : ABCOp0<"and2">;
def AND4 : ABCOp0<"and4">;
def OR : ABCOp0<"or">;
def OR2 : ABCOp0<"or2">;
def OR4 : ABCOp0<"or4">;
def XOR : ABCOp0<"xor">;
def XOR2 : ABCOp0<"xor2">;
def XOR4 : ABCOp0<"xor4">;
def COMP : ABCOp0<"comp">;
def COMP2 : ABCOp0<"comp2">;
def COMP4 : ABCOp0<"comp4">;
def BOOL : ABCOp0<"bool">;
def BOOL2 : ABCOp0<"bool2">;
def BOOL3 : ABCOp0<"bool3">;
def BOOL4 : ABCOp0<"bool4">;
def CULT : ABCOp0<"cult">;
def CULT2 : ABCOp0<"cult2">;
def CULT3 : ABCOp0<"cult3">;
def CULT4 : ABCOp0<"cult4">;
def CSLT : ABCOp0<"cslt">;
def CSLT2 : ABCOp0<"cslt2">;
def CSLT3 : ABCOp0<"cslt3">;
def CSLT4 : ABCOp0<"cslt4">;
def CFEQ : ABCOp0<"cfeq">;
def CFLT : ABCOp0<"cflt">;
def NOT : ABCOp0<"not">;
def FADD : ABCOp0<"fadd">;
def FSUB : ABCOp0<"fsub">;
def FMUL : ABCOp0<"fmul">;
def FDIV : ABCOp0<"fdiv">;
def F2I : ABCOp0<"f2i">;
def F2U : ABCOp0<"f2u">;
def I2F : ABCOp0<"i2f">;
def U2F : ABCOp0<"u2f">;

let isTerminator = 1 in {
def BZ : ABCOp24<"bz">;
def BZ1 : ABCOp8<"bz1">;
def BZ2 : ABCOp16<"bz2">;
def BNZ : ABCOp24<"bnz">;
def BNZ1 : ABCOp8<"bnz1">;
def BNZ2 : ABCOp16<"bnz2">;
def BZP : ABCOp24<"bzp">;
def BZP1 : ABCOp8<"bzp1">;
def BNZP : ABCOp24<"bnzp">;
def BNZP1 : ABCOp8<"bnzp1">;
}

let isTerminator = 1, isBarrier = 1 in {
def JMP : ABCOp24<"jmp">;
def JMP1 : ABCOp8<"jmp1">;
def JMP2 : ABCOp16<"jmp2">;
def IJMP : ABCOp0<"ijmp">;
}

let isCall = 1 in {
def CALL : ABCOp24<"call">;
def CALL1 : ABCOp8<"call1">;
def CALL2 : ABCOp16<"call2">;
def ICALL : ABCOp0<"icall">;
}

let isReturn = 1, isTerminator = 1, isBarrier = 1 in
def RET : ABCOp0<"ret">;

def SYS : ABCOp8<"sys">;
