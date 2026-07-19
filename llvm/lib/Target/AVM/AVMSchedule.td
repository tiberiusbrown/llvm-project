//===-- AVMSchedule.td - AVM interpreter schedule ------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

def AVMInterpreterModel : SchedMachineModel {
  let IssueWidth = 1;
  let CompleteModel = true;
}

let SchedModel = AVMInterpreterModel in {
  def AVMInterpreter : ProcResource<1> { let BufferSize = 1; }

  class AVMWrite<int Cycles> : SchedWriteRes<[AVMInterpreter]> {
    let Latency = Cycles;
    let ReleaseAtCycles = [Cycles];
  }

  def AVMW17 : AVMWrite<17>;
  def AVMW18 : AVMWrite<18>;
  def AVMW34 : AVMWrite<34>;
  def AVMW35 : AVMWrite<35>;
  def AVMW36 : AVMWrite<36>;
  def AVMW37 : AVMWrite<37>;
  def AVMW38 : AVMWrite<38>;
  def AVMW39 : AVMWrite<39>;
  def AVMW40 : AVMWrite<40>;
  def AVMW42 : AVMWrite<42>;
  def AVMW45 : AVMWrite<45>;
  def AVMW46 : AVMWrite<46>;
  def AVMW47 : AVMWrite<47>;
  def AVMW51 : AVMWrite<51>;
  def AVMW52 : AVMWrite<52>;
  def AVMW53 : AVMWrite<53>;
  def AVMW54 : AVMWrite<54>;
  def AVMW55 : AVMWrite<55>;
  def AVMW60 : AVMWrite<60>;
  def AVMW62 : AVMWrite<62>;
  def AVMW64 : AVMWrite<64>;
  def AVMW68 : AVMWrite<68>;
  def AVMW69 : AVMWrite<69>;
  def AVMW70 : AVMWrite<70>;
  def AVMW71 : AVMWrite<71>;
  def AVMW72 : AVMWrite<72>;
  def AVMW73 : AVMWrite<73>;
  def AVMW74 : AVMWrite<74>;
  def AVMW76 : AVMWrite<76>;
  def AVMW77 : AVMWrite<77>;
  def AVMW81 : AVMWrite<81>;
  def AVMW82 : AVMWrite<82>;
  def AVMW83 : AVMWrite<83>;
  def AVMW110 : AVMWrite<110>;
  def AVMW115 : AVMWrite<115>;
  def AVMW117 : AVMWrite<117>;
  def AVMW120 : AVMWrite<120>;
  def AVMW122 : AVMWrite<122>;
  def AVMW128 : AVMWrite<128>;
  def AVMW135 : AVMWrite<135>;
  def AVMW140 : AVMWrite<140>;
  def AVMW150 : AVMWrite<150>;
  def AVMW175 : AVMWrite<175>;
  def AVMW180 : AVMWrite<180>;
  def AVMW190 : AVMWrite<190>;
  def AVMW200 : AVMWrite<200>;
  def AVMW205 : AVMWrite<205>;
  def AVMW219 : AVMWrite<219>;
  def AVMW225 : AVMWrite<225>;
  def AVMW229 : AVMWrite<229>;
  def AVMW290 : AVMWrite<290>;
  def AVMW292 : AVMWrite<292>;
  def AVMW306 : AVMWrite<306>;
  def AVMW307 : AVMWrite<307>;
  def AVMW323 : AVMWrite<323>;
  def AVMW326 : AVMWrite<326>;
  def AVMW340 : AVMWrite<340>;
  def AVMW343 : AVMWrite<343>;
  def AVMW357 : AVMWrite<357>;
  def AVMW560 : AVMWrite<560>;

  def : InstRW<[AVMW17], (instrs MOV, ADD, SUB, AND, OR, XOR)>;
  def : InstRW<[AVMW18], (instrs CMP, LD8U, ST8, LD16, ST16, PUSH16, POP16)>;
  def : InstRW<[AVMW37], (instrs MOV_RR, ZEXT8, SWAP8, GETSP, SETSP)>;
  def : InstRW<[AVMW38], (instrs ADD_RR, SUB_RR, ADD32, SUB32, LSR32_1,
                                  ASR32_1, BOOL, INC16, DEC16, LSL16_1,
                                  LSR16_1, ASR16_1, NOT16, BSWAP16, MUL8,
                                  SEXT8, NEG16, TST8, CSET_EQ, CSET_NE,
                                  CSET_ULT, CSET_UGE, CSET_SLT, CSET_SGE,
                                  F3ST8, F6ST8_POST, F5LD8U, MULU8W, MULS8W,
                                  MULSU8W, LDSP8U_COMPACT, LDSP16_COMPACT,
                                  STSP16_COMPACT)>;
  def : InstRW<[AVMW39], (instrs CMP_RR, TST16, F5LD16, F5ST16,
                                  F7LD8U_POST, F7ST16_POST)>;
  def : InstRW<[AVMW40], (instrs F7LD16_POST)>;
  def : InstRW<[AVMW46], (instrs MUL16, FNEG)>;
  def : InstRW<[AVMW52], (instrs AND_RR, LDI16)>;
  def : InstRW<[AVMW53], (instrs OR_RR, LDI8)>;
  def : InstRW<[AVMW54], (instrs XOR_RR, ADDIS8)>;
  def : InstRW<[AVMW55], (instrs CMPIS8)>;
  def : InstRW<[AVMW60], (instrs LEASP, STSP8)>;
  def : InstRW<[AVMW62], (instrs LDSP8U)>;
  def : InstRW<[AVMW64], (instrs LDSP8S, LDSP16, STSP16)>;
  def : InstRW<[AVMW68], (instrs GPST8)>;
  def : InstRW<[AVMW69], (instrs GPST16)>;
  def : InstRW<[AVMW70], (instrs GPLD8U)>;
  def : InstRW<[AVMW71], (instrs GPLD16, COLDLDI16)>;
  def : InstRW<[AVMW72], (instrs LDM8U, STM8, LDM16, STM16)>;
  def : InstRW<[AVMW73], (instrs GPST8_POST)>;
  def : InstRW<[AVMW74], (instrs MOV32_F2, GPST16_POST)>;
  def : InstRW<[AVMW76], (instrs GPLD8U_POST)>;
  def : InstRW<[AVMW77], (instrs GPLD16_POST)>;
  def : InstRW<[AVMW81], (instrs ST32)>;
  def : InstRW<[AVMW82], (instrs LD32)>;
  def : InstRW<[AVMW83], (instrs CMP32)>;
  def : InstRW<[AVMW110], (instrs JMPP, RET)>;
  def : InstRW<[AVMW115], (instrs FMIN, FMAX)>;
  def : InstRW<[AVMW115], (instrs FTRUNC)>;
  def : InstRW<[AVMW120], (instrs FFLOOR, FCEIL, FROUND)>;
  def : InstRW<[AVMW45], (instrs FABS)>;
  def : InstRW<[AVMW47], (instrs LSL16I, LSR16I, ASR16I)>;
  def : InstRW<[AVMW117], (instrs CALLP)>;
  def : InstRW<[AVMW122], (instrs JMP8)>;
  def : InstRW<[AVMW128], (instrs CALL8)>;
  def : InstRW<[AVMW135], (instrs JMP16, CALL16)>;
  def : InstRW<[AVMW135], (instrs FCLASS)>;
  def : InstRW<[AVMW140], (instrs FCMP)>;
  def : InstRW<[AVMW150], (instrs JMPF, CALLF)>;
  def : InstRW<[AVMW175], (instrs U16TOF)>;
  def : InstRW<[AVMW180], (instrs S16TOF, FTOU16)>;
  def : InstRW<[AVMW190], (instrs FADD, FSUB, FTOS16, FTOU32)>;
  def : InstRW<[AVMW200], (instrs S32TOF, U32TOF)>;
  def : InstRW<[AVMW205], (instrs FTOS32)>;
  def : InstRW<[AVMW219], (instrs UDIV16, UREM16)>;
  def : InstRW<[AVMW225], (instrs FMUL)>;
  def : InstRW<[AVMW229], (instrs SDIV16, SREM16)>;
  def : InstRW<[AVMW290], (instrs LDP8U)>;
  def : InstRW<[AVMW292], (instrs LDP8S)>;
  def : InstRW<[AVMW306], (instrs LDP16)>;
  def : InstRW<[AVMW307], (instrs LDP8U_POST)>;
  def : InstRW<[AVMW323], (instrs LDP16_POST)>;
  def : InstRW<[AVMW326], (instrs LDP24)>;
  def : InstRW<[AVMW340], (instrs LDP32)>;
  def : InstRW<[AVMW343], (instrs LDP24_POST)>;
  def : InstRW<[AVMW357], (instrs LDP32_POST)>;
  def : InstRW<[AVMW560], (instrs FDIV, FSQRT)>;
  def : InstRW<[AVMW42], (instrs SHL16V, LSR16V, ASR16V)>;
  def : InstRW<[AVMW37], (instrs STSP8_COMPACT)>;
  def : InstRW<[AVMW53], (instrs COLDLDI8)>;
  def : InstRW<[AVMW54], (instrs COLDADDIS8)>;
  def : InstRW<[AVMW55], (instrs COLDCMPIS8)>;
  def : InstRW<[AVMW35], (instrs ADJSP)>;
  def : InstRW<[AVMW34], (instrs SYS)>;
  def : InstRW<[AVMW17], (instrs COPY)>;

  // Outcome-dependent branches use their not-taken cost because they end a
  // basic block. Branch-probability clients use AVMCostModel directly.
  def : InstRW<[AVMW35], (instrs BREQ8, BRNE8, BRULT8, BRUGE8, BRSLT8,
                                  BRSGE8)>;
  def : InstRW<[AVMW51], (instrs BREQ16, BRNE16, BRULT16, BRUGE16, BRSLT16,
                                  BRSGE16)>;
  def : InstRW<[AVMW36], (instrs CMOV_EQ, CMOV_NE, CMOV_ULT, CMOV_UGE,
                                  CMOV_SLT, CMOV_SGE)>;
}

def : ProcessorModel<"avm-interpreter-32u4-v1", AVMInterpreterModel,
                     [FeatureV1]>;
