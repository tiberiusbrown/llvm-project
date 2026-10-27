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

// Generated from the checked-in instruction calibration; see AVMCycleCosts.def.
  def AVMW17 : AVMWrite<17>;
  def AVMW18 : AVMWrite<18>;
  def AVMW19 : AVMWrite<19>;
  def AVMW35 : AVMWrite<35>;
  def AVMW36 : AVMWrite<36>;
  def AVMW37 : AVMWrite<37>;
  def AVMW38 : AVMWrite<38>;
  def AVMW39 : AVMWrite<39>;
  def AVMW40 : AVMWrite<40>;
  def AVMW41 : AVMWrite<41>;
  def AVMW42 : AVMWrite<42>;
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
  def AVMW71 : AVMWrite<71>;
  def AVMW72 : AVMWrite<72>;
  def AVMW73 : AVMWrite<73>;
  def AVMW74 : AVMWrite<74>;
  def AVMW76 : AVMWrite<76>;
  def AVMW77 : AVMWrite<77>;
  def AVMW81 : AVMWrite<81>;
  def AVMW82 : AVMWrite<82>;
  def AVMW83 : AVMWrite<83>;
  def AVMW102 : AVMWrite<102>;
  def AVMW105 : AVMWrite<105>;
  def AVMW110 : AVMWrite<110>;
  def AVMW112 : AVMWrite<112>;
  def AVMW114 : AVMWrite<114>;
  def AVMW115 : AVMWrite<115>;
  def AVMW122 : AVMWrite<122>;
  def AVMW123 : AVMWrite<123>;
  def AVMW125 : AVMWrite<125>;
  def AVMW126 : AVMWrite<126>;
  def AVMW131 : AVMWrite<131>;
  def AVMW139 : AVMWrite<139>;
  def AVMW140 : AVMWrite<140>;
  def AVMW154 : AVMWrite<154>;
  def AVMW156 : AVMWrite<156>;
  def AVMW192 : AVMWrite<192>;
  def AVMW219 : AVMWrite<219>;
  def AVMW229 : AVMWrite<229>;
  def AVMW285 : AVMWrite<285>;
  def AVMW287 : AVMWrite<287>;
  def AVMW301 : AVMWrite<301>;
  def AVMW302 : AVMWrite<302>;
  def AVMW318 : AVMWrite<318>;
  def AVMW321 : AVMWrite<321>;
  def AVMW335 : AVMWrite<335>;
  def AVMW338 : AVMWrite<338>;
  def AVMW352 : AVMWrite<352>;
  def AVMW529 : AVMWrite<529>;

  // Branches use not-taken costs; variable shifts use count zero,
  // immediate shifts count one. Operand-sensitive forms use their
  // existing representative encoding; AVMInstrInfo refines actual operands.
  def : InstRW<[AVMW17], (instrs MOV, ADD, SUB, AND, OR, XOR, COPY)>;
  def : InstRW<[AVMW18], (instrs CMP, LD8U, ST8, LD16, ST16, POP16)>;
  def : InstRW<[AVMW19], (instrs PUSH16)>;
  def : InstRW<[AVMW35], (instrs ADJSP, SYS, BREQ8, BRNE8, BRULT8, BRUGE8,
                                  BRSLT8, BRSGE8)>;
  def : InstRW<[AVMW36], (instrs CMOV_EQ, CMOV_NE, CMOV_ULT, CMOV_UGE,
                                  CMOV_SLT, CMOV_SGE)>;
  def : InstRW<[AVMW37], (instrs MOV_RR, ZEXT8, SWAP8, GETSP, STSP8_COMPACT)>;
  def : InstRW<[AVMW38], (instrs ADD_RR, SUB_RR, ADD32, SUB32, LSR32_1,
                                  ASR32_1, BOOL, INC16, DEC16, LSL16_1,
                                  LSR16_1, ASR16_1, NOT16, BSWAP16, MUL8,
                                  SEXT8, NEG16, TST8, F3ST8, F6ST8_POST,
                                  F5LD8U, MULU8W, MULS8W, MULSU8W,
                                  LDSP8U_COMPACT, LDSP16_COMPACT,
                                  STSP16_COMPACT, MOV32_F2, FNEG, FABS)>;
  def : InstRW<[AVMW39], (instrs CMP_RR, TST16, F5LD16, F5ST16, F7LD8U_POST,
                                  F7ST16_POST)>;
  def : InstRW<[AVMW40], (instrs SETSP, F7LD16_POST)>;
  def : InstRW<[AVMW41], (instrs CSET_EQ, CSET_NE, CSET_ULT, CSET_UGE,
                                  CSET_SLT, CSET_SGE)>;
  def : InstRW<[AVMW42], (instrs SHL16V, LSR16V, ASR16V)>;
  def : InstRW<[AVMW46], (instrs MUL16)>;
  def : InstRW<[AVMW47], (instrs LSL16I, LSR16I, ASR16I)>;
  def : InstRW<[AVMW51], (instrs AND_RR, BREQ16, BRNE16, BRULT16, BRUGE16,
                                  BRSLT16, BRSGE16)>;
  def : InstRW<[AVMW52], (instrs LDI16, OR_RR)>;
  def : InstRW<[AVMW53], (instrs LDI8, XOR_RR, COLDLDI8)>;
  def : InstRW<[AVMW54], (instrs ADDIS8, DPLD8U, DPST8, COLDADDIS8)>;
  def : InstRW<[AVMW55], (instrs CMPIS8, DPLD16, DPST16, COLDCMPIS8)>;
  def : InstRW<[AVMW60], (instrs LEASP, STSP8)>;
  def : InstRW<[AVMW62], (instrs LDSP8U)>;
  def : InstRW<[AVMW64], (instrs LDSP8S, LDSP16, STSP16)>;
  def : InstRW<[AVMW71], (instrs COLDLDI16)>;
  def : InstRW<[AVMW72], (instrs LDM8U, STM8, LDM16, STM16)>;
  def : InstRW<[AVMW73], (instrs GPST8_POST)>;
  def : InstRW<[AVMW74], (instrs GPST16_POST)>;
  def : InstRW<[AVMW76], (instrs GPLD8U_POST)>;
  def : InstRW<[AVMW77], (instrs GPLD16_POST)>;
  def : InstRW<[AVMW81], (instrs ST32)>;
  def : InstRW<[AVMW82], (instrs LD32)>;
  def : InstRW<[AVMW83], (instrs CMP32, FCMP, FCMP_MC)>;
  def : InstRW<[AVMW102], (instrs FTOU16, FTOU16_MC)>;
  def : InstRW<[AVMW105], (instrs FTOU32, FTOU32_MC)>;
  def : InstRW<[AVMW110], (instrs FCLASS, FCLASS_MC)>;
  def : InstRW<[AVMW112], (instrs FTOS16, FTOS16_MC)>;
  def : InstRW<[AVMW114], (instrs JMPP, RET)>;
  def : InstRW<[AVMW115], (instrs FTOS32, FTOS32_MC)>;
  def : InstRW<[AVMW122], (instrs S32TOF, S32TOF_MC, U32TOF, U32TOF_MC)>;
  def : InstRW<[AVMW123], (instrs CALLP, U16TOF, U16TOF_MC)>;
  def : InstRW<[AVMW125], (instrs S16TOF, S16TOF_MC)>;
  def : InstRW<[AVMW126], (instrs JMP8)>;
  def : InstRW<[AVMW131], (instrs CALL8)>;
  def : InstRW<[AVMW139], (instrs JMP16)>;
  def : InstRW<[AVMW140], (instrs CALL16)>;
  def : InstRW<[AVMW154], (instrs JMPF, CALLF)>;
  def : InstRW<[AVMW156], (instrs FADD, FADD_MC, FSUB, FSUB_MC)>;
  def : InstRW<[AVMW192], (instrs FMUL, FMUL_MC)>;
  def : InstRW<[AVMW219], (instrs UDIV16, UREM16)>;
  def : InstRW<[AVMW229], (instrs SDIV16, SREM16)>;
  def : InstRW<[AVMW285], (instrs LDP8U)>;
  def : InstRW<[AVMW287], (instrs LDP8S)>;
  def : InstRW<[AVMW301], (instrs LDP16)>;
  def : InstRW<[AVMW302], (instrs LDP8U_POST)>;
  def : InstRW<[AVMW318], (instrs LDP16_POST)>;
  def : InstRW<[AVMW321], (instrs LDP24)>;
  def : InstRW<[AVMW335], (instrs LDP32)>;
  def : InstRW<[AVMW338], (instrs LDP24_POST)>;
  def : InstRW<[AVMW352], (instrs LDP32_POST)>;
  def : InstRW<[AVMW529], (instrs FDIV, FDIV_MC)>;

}

def : ProcessorModel<"avm-interpreter-32u4-v1", AVMInterpreterModel,
                     [FeatureV1]>;
