//===-- AVMInstrInfo.td - AVM instruction definitions -------*- tablegen -*-===//

// These records describe concrete Version 1 instructions for the MC layer.
// Code-generation operand ties are deliberately deferred until GlobalISel
// bring-up; the explicit operands here match assembly order exactly.

def AVMImm8   : Operand<i16>;
def AVMImm16  : Operand<i16>;
def AVMImm24  : Operand<i32>;
def AVMDisp8  : Operand<i16>;
def AVMOffset : Operand<i16>;

class AVMInst<string Asm, int Bytes, dag Operands = (ins)> : Instruction {
  let Namespace = "AVM";
  let OutOperandList = (outs);
  let InOperandList = Operands;
  let AsmString = Asm;
  let Size = Bytes;
  let isCodeGenOnly = 0;
}

class AVMFlagDefInst<string Asm, int Bytes, dag Operands = (ins)>
    : AVMInst<Asm, Bytes, Operands> {
  let Defs = [FLAGS];
}
class AVMFlagUseDefInst<string Asm, int Bytes, dag Operands = (ins)>
    : AVMFlagDefInst<Asm, Bytes, Operands> {
  let Uses = [FLAGS];
}
class AVMBranchInst<string Asm, int Bytes>
    : AVMInst<Asm, Bytes, (ins AVMDisp8:$target)> {
  let isBranch = 1;
  let isTerminator = 1;
  let Uses = [FLAGS];
}
class AVMCallInst<string Asm, int Bytes, dag Operands>
    : AVMInst<Asm, Bytes, Operands> {
  let isCall = 1;
}

// Primary compact operations.
def CLR      : AVMInst<"clr $dst", 1, (ins CGPR16:$dst)>;
def MOVC     : AVMInst<"mov $dst, $src", 1, (ins CGPR16:$dst, CGPR16:$src)>;
def LD8C     : AVMInst<"ld8 $dst, [$addr]", 1, (ins CGPR16:$dst, CGPR16:$addr)> { let mayLoad = 1; }
def ST8C     : AVMInst<"st8 [$addr], $src", 1, (ins CGPR16:$addr, CGPR16:$src)> { let mayStore = 1; }
def LD16C    : AVMInst<"ld16 $dst, [$addr]", 1, (ins CGPR16:$dst, CGPR16:$addr)> { let mayLoad = 1; }
def ST16C    : AVMInst<"st16 [$addr], $src", 1, (ins CGPR16:$addr, CGPR16:$src)> { let mayStore = 1; }
def PUSH16   : AVMInst<"push16 $src", 1, (ins GPR16:$src)> { let mayStore = 1; }
def POP16    : AVMInst<"pop16 $dst", 1, (ins GPR16:$dst)> { let mayLoad = 1; }
def ADDC     : AVMFlagDefInst<"add $dst, $src", 1, (ins CGPR16:$dst, CGPR16:$src)>;
def SUBC     : AVMFlagDefInst<"sub $dst, $src", 1, (ins CGPR16:$dst, CGPR16:$src)>;
def CMP16C   : AVMFlagDefInst<"cmp16 $lhs, $rhs", 1, (ins CGPR16:$lhs, CGPR16:$rhs)>;
def TST16C   : AVMFlagDefInst<"tst16 $src", 1, (ins CGPR16:$src)>;
def CMP8C    : AVMFlagDefInst<"cmp8 $lhs, $rhs", 1, (ins CGPR16:$lhs, CGPR16:$rhs)>;
def TST8C    : AVMFlagDefInst<"tst8 $src", 1, (ins CGPR16:$src)>;
def BEQ_SHORT : AVMBranchInst<"beq.s $target", 1>;
def BNE_SHORT : AVMBranchInst<"bne.s $target", 1>;
def INC16    : AVMInst<"inc16 $dst", 2, (ins GPR16:$dst)>;
def DEC16    : AVMInst<"dec16 $dst", 2, (ins GPR16:$dst)>;
def LDI8C    : AVMInst<"ldi8 $dst, $imm", 2, (ins CGPR16:$dst, AVMImm8:$imm)>;
def ADDNF    : AVMInst<"add.nf $dst, $src", 2, (ins CGPR16:$dst, CGPR16:$src)>;
def SUBNF    : AVMInst<"sub.nf $dst, $src", 2, (ins CGPR16:$dst, CGPR16:$src)>;

// 0xE1 aligned 32-bit register-pair operations.
def MOV32  : AVMInst<"mov32 $dst, $src", 2,
                      (ins GPR32:$dst, GPR32:$src)>;
def ADD32  : AVMInst<"add32 $dst, $src", 2,
                      (ins GPR32:$dst, GPR32:$src)>;
def SUB32  : AVMInst<"sub32 $dst, $src", 2,
                      (ins GPR32:$dst, GPR32:$src)>;
def AND32  : AVMInst<"and32 $dst, $src", 2,
                      (ins GPR32:$dst, GPR32:$src)>;
def OR32   : AVMInst<"or32 $dst, $src", 2,
                      (ins GPR32:$dst, GPR32:$src)>;
def XOR32  : AVMInst<"xor32 $dst, $src", 2,
                      (ins GPR32:$dst, GPR32:$src)>;
def CMP32  : AVMFlagDefInst<"cmp32 $lhs, $rhs", 2,
                             (ins GPR32:$lhs, GPR32:$rhs)>;
def SHL32V : AVMInst<"shl32v $dst, $src", 2,
                      (ins GPR32:$dst, GPR32:$src)>;
def LSR32V : AVMInst<"lsr32v $dst, $src", 2,
                      (ins GPR32:$dst, GPR32:$src)>;
def ASR32V : AVMInst<"asr32v $dst, $src", 2,
                      (ins GPR32:$dst, GPR32:$src)>;

// 0xE3 full-register transfers and condition materialization.
def MOV16_E3 : AVMInst<"mov16 $dst, $src", 2, (ins GPR16:$dst, GPR16:$src)>;
def MOV8Z    : AVMInst<"mov8z $dst, $src", 2, (ins GPR16:$dst, GPR8:$src)>;
def MOV8S    : AVMInst<"mov8s $dst, $src", 2, (ins GPR16:$dst, GPR8:$src)>;
def CSET     : AVMInst<"cset $dst, $cc", 2,
                        (ins GPR16:$dst, AVMImm8:$cc)> { let Uses = [FLAGS]; }

// General conditional branches.
def BREQ  : AVMBranchInst<"breq $target", 2>;
def BRNE  : AVMBranchInst<"brne $target", 2>;
def BRULT : AVMBranchInst<"brult $target", 2>;
def BRUGE : AVMBranchInst<"bruge $target", 2>;
def BRSLT : AVMBranchInst<"brslt $target", 2>;
def BRSGE : AVMBranchInst<"brsge $target", 2>;
def BRULE : AVMBranchInst<"brule $target", 2>;
def BRUGT : AVMBranchInst<"brugt $target", 2>;

// 0xE0 unary operations.
def NOT16 : AVMInst<"not16 $dst", 2, (ins GPR16:$dst)>;
def NEG16 : AVMInst<"neg16 $dst", 2, (ins GPR16:$dst)>;
def LSL16 : AVMInst<"lsl16 $dst", 2, (ins NGPR16:$dst)>;
def LSR16 : AVMInst<"lsr16 $dst", 2, (ins GPR16:$dst)>;
def ASR16 : AVMInst<"asr16 $dst", 2, (ins GPR16:$dst)>;
def LSR8  : AVMInst<"lsr8 $dst", 2, (ins GPR16:$dst)>;
def ASR8  : AVMInst<"asr8 $dst", 2, (ins GPR16:$dst)>;
def SWAP8 : AVMInst<"swap8 $dst", 2, (ins GPR16:$dst)>;
def GETSP : AVMInst<"getsp $dst", 2, (ins GPR16:$dst)>;
def SETSP : AVMInst<"setsp $src", 2, (ins GPR16:$src)>;

// One-byte accumulator logical operations.  These accept any source register.
def ANDA : AVMInst<"and $dst, $src", 1, (ins AGPR16:$dst, GPR16:$src)>;
def ORA  : AVMInst<"or $dst, $src", 1, (ins AGPR16:$dst, GPR16:$src)>;
def XORA : AVMInst<"xor $dst, $src", 1, (ins AGPR16:$dst, GPR16:$src)>;
def BICA : AVMInst<"bic $dst, $src", 1, (ins AGPR16:$dst, GPR16:$src)>;

// 0xF4 binary operations.  Logical destination c0/A is reserved because the
// one-byte accumulator forms above cover the same operations.
def AND16 : AVMInst<"and $dst, $src", 2,
                     (ins CGPR16NoA:$dst, CGPR16:$src)>;
def OR16  : AVMInst<"or $dst, $src", 2,
                     (ins CGPR16NoA:$dst, CGPR16:$src)>;
def XOR16 : AVMInst<"xor $dst, $src", 2,
                     (ins CGPR16NoA:$dst, CGPR16:$src)>;
def BIC16 : AVMInst<"bic $dst, $src", 2,
                     (ins CGPR16NoA:$dst, CGPR16:$src)>;
def ADC16  : AVMFlagUseDefInst<"adc $dst, $src", 3, (ins GPR16:$dst, GPR16:$src)>;
def SBC16  : AVMFlagUseDefInst<"sbc $dst, $src", 3, (ins GPR16:$dst, GPR16:$src)>;
def CMP8   : AVMFlagDefInst<"cmp8 $lhs, $rhs", 3, (ins GPR16:$lhs, GPR16:$rhs)>;
def CPC16  : AVMFlagUseDefInst<"cpc16 $lhs, $rhs", 3, (ins GPR16:$lhs, GPR16:$rhs)>;
def MULU8  : AVMInst<"mulu8 $dst, $src", 2, (ins GPR16:$dst, GPR16:$src)>;
def MULS8  : AVMInst<"muls8 $dst, $src", 2, (ins GPR16:$dst, GPR16:$src)>;
def MULSU8 : AVMInst<"mulsu8 $dst, $src", 2, (ins GPR16:$dst, GPR16:$src)>;
def SHL16V : AVMInst<"shl16v $dst, $src", 2, (ins GPR16:$dst, GPR16:$src)>;
def LSR16V : AVMInst<"lsr16v $dst, $src", 2, (ins GPR16:$dst, GPR16:$src)>;
def ASR16V : AVMInst<"asr16v $dst, $src", 2, (ins GPR16:$dst, GPR16:$src)>;

// 0xE0 immediate operations.
def LDI16  : AVMInst<"ldi16 $dst, $imm", 4, (ins GPR16:$dst, AVMImm16:$imm)>;
def LDI8   : AVMInst<"ldi8 $dst, $imm", 3, (ins NGPR16:$dst, AVMImm8:$imm)>;
def ADDI16 : AVMInst<"addi16 $dst, $imm", 4, (ins GPR16:$dst, AVMImm16:$imm)>;
def SUBI16 : AVMInst<"subi16 $dst, $imm", 4, (ins GPR16:$dst, AVMImm16:$imm)>;
def ANDI16 : AVMInst<"andi16 $dst, $imm", 4, (ins GPR16:$dst, AVMImm16:$imm)>;
def ORI16  : AVMInst<"ori16 $dst, $imm", 4, (ins GPR16:$dst, AVMImm16:$imm)>;
def XORI16 : AVMInst<"xori16 $dst, $imm", 4, (ins GPR16:$dst, AVMImm16:$imm)>;
def CMPI16 : AVMFlagDefInst<"cmpi16 $src, $imm", 4, (ins GPR16:$src, AVMImm16:$imm)>;
def CMPI8  : AVMFlagDefInst<"cmpi8 $src, $imm", 3, (ins GPR16:$src, AVMImm8:$imm)>;

// E0 tests exist only for r0-r3. Compact tests use the diagonal primary
// compare encodings represented by TST16C/TST8C above.
def TST16 : AVMFlagDefInst<"tst16 $src", 2, (ins NGPR16:$src)>;
def TST8  : AVMFlagDefInst<"tst8 $src", 2, (ins NGPR16:$src)>;

// Relocated core operations and compact stack forms.
def MOV16     : AVMInst<"mov $dst, $src", 3, (ins GPR16:$dst, GPR16:$src)>;
def ADD16     : AVMFlagDefInst<"add $dst, $src", 3, (ins GPR16:$dst, GPR16:$src)>;
def SUB16     : AVMFlagDefInst<"sub $dst, $src", 3, (ins GPR16:$dst, GPR16:$src)>;
def CMP16     : AVMFlagDefInst<"cmp16 $lhs, $rhs", 3, (ins GPR16:$lhs, GPR16:$rhs)>;
def CMPI6     : AVMFlagDefInst<"cmpi6 $src, $imm", 2, (ins CGPR16:$src, AVMImm8:$imm)>;
def JMP_REL8  : AVMInst<"jmp $target", 2, (ins AVMDisp8:$target)> { let isBranch = 1; let isTerminator = 1; let isBarrier = 1; }
def CALL_REL8 : AVMCallInst<"call $target", 2, (ins AVMDisp8:$target)>;
def ADJSP     : AVMInst<"adjsp $amount", 2, (ins AVMDisp8:$amount)>;
def LDSP8C    : AVMInst<"ldsp8 $dst, [sp+$off]", 3, (ins CGPR16:$dst, AVMOffset:$off)> { let mayLoad = 1; }
def LDSP16C   : AVMInst<"ldsp16 $dst, [sp+$off]", 3, (ins CGPR16:$dst, AVMOffset:$off)> { let mayLoad = 1; }
def STSP8C    : AVMInst<"stsp8 [sp+$off], $src", 3, (ins AVMOffset:$off, CGPR16:$src)> { let mayStore = 1; }
def STSP16C   : AVMInst<"stsp16 [sp+$off], $src", 3, (ins AVMOffset:$off, CGPR16:$src)> { let mayStore = 1; }

// Indirect control and PB transfers.
def JMPR  : AVMInst<"jmpr $target", 2, (ins GPR16:$target)> { let isBranch = 1; let isTerminator = 1; let isBarrier = 1; }
def CALLR : AVMCallInst<"callr $target", 2, (ins GPR16:$target)>;
def JMPP  : AVMInst<"jmpp $target", 2, (ins GPR16:$target)> { let isBranch = 1; let isTerminator = 1; let isBarrier = 1; let Uses = [PB]; }
def CALLP : AVMCallInst<"callp $target", 2, (ins GPR16:$target)> { let Uses = [PB]; }
def MTPB  : AVMInst<"mtpb $src", 2, (ins GPR16:$src)> { let Defs = [PB]; }
def MFPB  : AVMInst<"mfpb $dst", 2, (ins GPR16:$dst)> { let Uses = [PB]; }
def LDPBI : AVMInst<"ldpbi $imm", 2, (ins AVMImm8:$imm)> { let Defs = [PB]; }
def JMP16 : AVMInst<"jmp16 $target", 3, (ins AVMImm16:$target)> { let isBranch = 1; let isTerminator = 1; let isBarrier = 1; }
def CALL16 : AVMCallInst<"call16 $target", 3, (ins AVMImm16:$target)>;
def NOP   : AVMInst<"nop", 1>;
def SYS   : AVMInst<"sys $service", 2, (ins AVMImm8:$service)> { let hasSideEffects = 1; }

// Generic data-space memory extension.
def LD8       : AVMInst<"ld8 $dst, [$addr]", 3, (ins GPR16:$dst, GPR16:$addr)> { let mayLoad = 1; }
def ST8       : AVMInst<"st8 [$addr], $src", 3, (ins GPR16:$addr, GPR16:$src)> { let mayStore = 1; }
def LD16      : AVMInst<"ld16 $dst, [$addr]", 3, (ins GPR16:$dst, GPR16:$addr)> { let mayLoad = 1; }
def ST16      : AVMInst<"st16 [$addr], $src", 3, (ins GPR16:$addr, GPR16:$src)> { let mayStore = 1; }
def LD8_POST  : AVMInst<"ld8_post $dst, [$addr]+", 3, (ins GPR16:$dst, GPR16:$addr)> { let mayLoad = 1; }
def ST8_POST  : AVMInst<"st8_post [$addr]+, $src", 3, (ins GPR16:$addr, GPR16:$src)> { let mayStore = 1; }
def LD16_POST : AVMInst<"ld16_post $dst, [$addr]+", 3, (ins GPR16:$dst, GPR16:$addr)> { let mayLoad = 1; }
def ST16_POST : AVMInst<"st16_post [$addr]+, $src", 3, (ins GPR16:$addr, GPR16:$src)> { let mayStore = 1; }
def LEA       : AVMInst<"lea $dst, [$addr+$disp]", 4, (ins GPR16:$dst, GPR16:$addr, AVMDisp8:$disp)>;
def LD8_DISP  : AVMInst<"ld8 $dst, [$addr+$disp]", 4, (ins GPR16:$dst, GPR16:$addr, AVMDisp8:$disp)> { let mayLoad = 1; }
def ST8_DISP  : AVMInst<"st8 [$addr+$disp], $src", 4, (ins GPR16:$addr, AVMDisp8:$disp, GPR16:$src)> { let mayStore = 1; }
def LD16_DISP : AVMInst<"ld16 $dst, [$addr+$disp]", 4, (ins GPR16:$dst, GPR16:$addr, AVMDisp8:$disp)> { let mayLoad = 1; }
def ST16_DISP : AVMInst<"st16 [$addr+$disp], $src", 4, (ins GPR16:$addr, AVMDisp8:$disp, GPR16:$src)> { let mayStore = 1; }

// Extended stack-relative forms.
def LDSP8  : AVMInst<"ldsp8 $dst, [sp+$off]", 3, (ins GPR16:$dst, AVMOffset:$off)> { let mayLoad = 1; }
def STSP8  : AVMInst<"stsp8 [sp+$off], $src", 3, (ins AVMOffset:$off, GPR16:$src)> { let mayStore = 1; }
def LDSP16 : AVMInst<"ldsp16 $dst, [sp+$off]", 3, (ins GPR16:$dst, AVMOffset:$off)> { let mayLoad = 1; }
def STSP16 : AVMInst<"stsp16 [sp+$off], $src", 3, (ins AVMOffset:$off, GPR16:$src)> { let mayStore = 1; }

// Direct data-space forms.
def LDM8  : AVMInst<"ldm8 $dst, $addr", 4, (ins GPR16:$dst, AVMImm16:$addr)> { let mayLoad = 1; }
def STM8  : AVMInst<"stm8 $addr, $src", 4, (ins AVMImm16:$addr, GPR16:$src)> { let mayStore = 1; }
def LDM16 : AVMInst<"ldm16 $dst, $addr", 4, (ins GPR16:$dst, AVMImm16:$addr)> { let mayLoad = 1; }
def STM16 : AVMInst<"stm16 $addr, $src", 4, (ins AVMImm16:$addr, GPR16:$src)> { let mayStore = 1; }

// Program-space loads.
def LDP8       : AVMInst<"ldp8 $dst, [pb:$addr]", 3, (ins GPR16:$dst, GPR16:$addr)> { let mayLoad = 1; let Uses = [PB]; }
def LDP16      : AVMInst<"ldp16 $dst, [pb:$addr]", 3, (ins GPR16:$dst, GPR16:$addr)> { let mayLoad = 1; let Uses = [PB]; }
def LDP8_DISP  : AVMInst<"ldp8 $dst, [pb:$addr+$disp]", 4, (ins GPR16:$dst, GPR16:$addr, AVMDisp8:$disp)> { let mayLoad = 1; let Uses = [PB]; }
def LDP16_DISP : AVMInst<"ldp16 $dst, [pb:$addr+$disp]", 4, (ins GPR16:$dst, GPR16:$addr, AVMDisp8:$disp)> { let mayLoad = 1; let Uses = [PB]; }

// Direct far control and return.
def JMPF : AVMInst<"jmpf $target", 4, (ins AVMImm24:$target)> { let isBranch = 1; let isTerminator = 1; let isBarrier = 1; }
def CALLF : AVMCallInst<"callf $target", 4, (ins AVMImm24:$target)>;
def RET : AVMInst<"ret", 1> { let isReturn = 1; let isTerminator = 1; let isBarrier = 1; }

// The only placeholder needed before GlobalISel begins.
def AVM_BACKEND_NOT_IMPLEMENTED : AVMInst<"<avm-backend-not-implemented>", 0> {
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}
