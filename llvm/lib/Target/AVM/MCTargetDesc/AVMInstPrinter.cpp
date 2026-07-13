#include "AVMInstPrinter.h"
#include "AVMMCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

std::pair<const char *, uint64_t>
AVMInstPrinter::getMnemonic(const MCInst &MI) const {
#define AVM_MNEMONIC(OP, NAME) case AVM::OP: return {NAME, 0}
  switch (MI.getOpcode()) {
  AVM_MNEMONIC(CLR, "clr");
  AVM_MNEMONIC(MOVC, "mov");
  AVM_MNEMONIC(MOV16, "mov");
  AVM_MNEMONIC(MOV16_E3, "mov16");
  AVM_MNEMONIC(MOV8Z, "mov8z");
  AVM_MNEMONIC(MOV8S, "mov8s");
  AVM_MNEMONIC(CSET, "cset");
  AVM_MNEMONIC(LD8C, "ld8");
  AVM_MNEMONIC(LD8, "ld8");
  AVM_MNEMONIC(LD8_DISP, "ld8");
  AVM_MNEMONIC(ST8C, "st8");
  AVM_MNEMONIC(ST8, "st8");
  AVM_MNEMONIC(ST8_DISP, "st8");
  AVM_MNEMONIC(LD16C, "ld16");
  AVM_MNEMONIC(LD16, "ld16");
  AVM_MNEMONIC(LD16_DISP, "ld16");
  AVM_MNEMONIC(ST16C, "st16");
  AVM_MNEMONIC(ST16, "st16");
  AVM_MNEMONIC(ST16_DISP, "st16");
  AVM_MNEMONIC(PUSH16, "push16");
  AVM_MNEMONIC(POP16, "pop16");
  AVM_MNEMONIC(ADDC, "add");
  AVM_MNEMONIC(ADD16, "add");
  AVM_MNEMONIC(SUBC, "sub");
  AVM_MNEMONIC(SUB16, "sub");
  AVM_MNEMONIC(CMP16C, "cmp16");
  AVM_MNEMONIC(CMP16, "cmp16");
  AVM_MNEMONIC(TST16, "tst16");
  AVM_MNEMONIC(CMP8C, "cmp8");
  AVM_MNEMONIC(CMP8, "cmp8");
  AVM_MNEMONIC(TST8, "tst8");
  AVM_MNEMONIC(BEQ_SHORT, "beq.s");
  AVM_MNEMONIC(BNE_SHORT, "bne.s");
  AVM_MNEMONIC(INC16, "inc16");
  AVM_MNEMONIC(DEC16, "dec16");
  AVM_MNEMONIC(LDI8C, "ldi8");
  AVM_MNEMONIC(ADDNF, "add.nf");
  AVM_MNEMONIC(SUBNF, "sub.nf");
  AVM_MNEMONIC(MOV32, "mov32");
  AVM_MNEMONIC(ADD32, "add32");
  AVM_MNEMONIC(SUB32, "sub32");
  AVM_MNEMONIC(AND32, "and32");
  AVM_MNEMONIC(OR32, "or32");
  AVM_MNEMONIC(XOR32, "xor32");
  AVM_MNEMONIC(CMP32, "cmp32");
  AVM_MNEMONIC(SHL32V, "shl32v");
  AVM_MNEMONIC(LSR32V, "lsr32v");
  AVM_MNEMONIC(ASR32V, "asr32v");
  AVM_MNEMONIC(BREQ, "breq");
  AVM_MNEMONIC(BRNE, "brne");
  AVM_MNEMONIC(BRULT, "brult");
  AVM_MNEMONIC(BRUGE, "bruge");
  AVM_MNEMONIC(BRSLT, "brslt");
  AVM_MNEMONIC(BRSGE, "brsge");
  AVM_MNEMONIC(BRULE, "brule");
  AVM_MNEMONIC(BRUGT, "brugt");
  AVM_MNEMONIC(NOT16, "not16");
  AVM_MNEMONIC(NEG16, "neg16");
  AVM_MNEMONIC(LSL16, "lsl16");
  AVM_MNEMONIC(LSR16, "lsr16");
  AVM_MNEMONIC(ASR16, "asr16");
  AVM_MNEMONIC(LSR8, "lsr8");
  AVM_MNEMONIC(ASR8, "asr8");
  AVM_MNEMONIC(ZEXT8, "zext8");
  AVM_MNEMONIC(SEXT8, "sext8");
  AVM_MNEMONIC(SWAP8, "swap8");
  AVM_MNEMONIC(GETSP, "getsp");
  AVM_MNEMONIC(SETSP, "setsp");
  AVM_MNEMONIC(ANDA, "and");
  AVM_MNEMONIC(ORA, "or");
  AVM_MNEMONIC(XORA, "xor");
  AVM_MNEMONIC(BICA, "bic");
  AVM_MNEMONIC(AND16, "and");
  AVM_MNEMONIC(OR16, "or");
  AVM_MNEMONIC(XOR16, "xor");
  AVM_MNEMONIC(BIC16, "bic");
  AVM_MNEMONIC(ADC16, "adc");
  AVM_MNEMONIC(SBC16, "sbc");
  AVM_MNEMONIC(CPC16, "cpc16");
  AVM_MNEMONIC(MULU8, "mulu8");
  AVM_MNEMONIC(MULS8, "muls8");
  AVM_MNEMONIC(MULSU8, "mulsu8");
  AVM_MNEMONIC(SHL16V, "shl16v");
  AVM_MNEMONIC(LSR16V, "lsr16v");
  AVM_MNEMONIC(ASR16V, "asr16v");
  AVM_MNEMONIC(LDI16, "ldi16");
  AVM_MNEMONIC(LDI8, "ldi8");
  AVM_MNEMONIC(ADDI16, "addi16");
  AVM_MNEMONIC(SUBI16, "subi16");
  AVM_MNEMONIC(ANDI16, "andi16");
  AVM_MNEMONIC(ORI16, "ori16");
  AVM_MNEMONIC(XORI16, "xori16");
  AVM_MNEMONIC(CMPI16, "cmpi16");
  AVM_MNEMONIC(CMPI8, "cmpi8");
  AVM_MNEMONIC(CMPI6, "cmpi6");
  AVM_MNEMONIC(JMP_REL8, "jmp");
  AVM_MNEMONIC(CALL_REL8, "call");
  AVM_MNEMONIC(ADJSP, "adjsp");
  AVM_MNEMONIC(LDSP8C, "ldsp8");
  AVM_MNEMONIC(LDSP16C, "ldsp16");
  AVM_MNEMONIC(STSP8C, "stsp8");
  AVM_MNEMONIC(STSP16C, "stsp16");
  AVM_MNEMONIC(JMPR, "jmpr");
  AVM_MNEMONIC(CALLR, "callr");
  AVM_MNEMONIC(JMPP, "jmpp");
  AVM_MNEMONIC(CALLP, "callp");
  AVM_MNEMONIC(MTPB, "mtpb");
  AVM_MNEMONIC(MFPB, "mfpb");
  AVM_MNEMONIC(LDPBI, "ldpbi");
  AVM_MNEMONIC(JMP16, "jmp16");
  AVM_MNEMONIC(CALL16, "call16");
  AVM_MNEMONIC(NOP, "nop");
  AVM_MNEMONIC(SYS, "sys");
  AVM_MNEMONIC(LD8_POST, "ld8_post");
  AVM_MNEMONIC(ST8_POST, "st8_post");
  AVM_MNEMONIC(LD16_POST, "ld16_post");
  AVM_MNEMONIC(ST16_POST, "st16_post");
  AVM_MNEMONIC(LEA, "lea");
  AVM_MNEMONIC(LDSP8, "ldsp8");
  AVM_MNEMONIC(STSP8, "stsp8");
  AVM_MNEMONIC(LDSP16, "ldsp16");
  AVM_MNEMONIC(STSP16, "stsp16");
  AVM_MNEMONIC(LDM8, "ldm8");
  AVM_MNEMONIC(STM8, "stm8");
  AVM_MNEMONIC(LDM16, "ldm16");
  AVM_MNEMONIC(STM16, "stm16");
  AVM_MNEMONIC(LDP8, "ldp8");
  AVM_MNEMONIC(LDP16, "ldp16");
  AVM_MNEMONIC(LDP8_DISP, "ldp8");
  AVM_MNEMONIC(LDP16_DISP, "ldp16");
  AVM_MNEMONIC(JMPF, "jmpf");
  AVM_MNEMONIC(CALLF, "callf");
  AVM_MNEMONIC(RET, "ret");
  default:
    return {"<unknown>", 0};
  }
#undef AVM_MNEMONIC
}

void AVMInstPrinter::printFullReg(MCRegister Reg, raw_ostream &OS) const {
  switch (Reg.id()) {
  case AVM::R0: OS << "r0"; return;
  case AVM::R1: OS << "r1"; return;
  case AVM::R2: OS << "r2"; return;
  case AVM::R3: OS << "r3"; return;
  case AVM::R4: OS << "r4"; return;
  case AVM::R5: OS << "r5"; return;
  case AVM::R6: OS << "r6"; return;
  case AVM::R7: OS << "r7"; return;
  case AVM::B0: OS << "b0"; return;
  case AVM::B1: OS << "b1"; return;
  case AVM::B2: OS << "b2"; return;
  case AVM::B3: OS << "b3"; return;
  case AVM::B4: OS << "b4"; return;
  case AVM::B5: OS << "b5"; return;
  case AVM::B6: OS << "b6"; return;
  case AVM::B7: OS << "b7"; return;
  case AVM::R0R1: OS << "q0"; return;
  case AVM::R2R3: OS << "q1"; return;
  case AVM::R4R5: OS << "q2"; return;
  case AVM::R6R7: OS << "q3"; return;
  case AVM::SP: OS << "sp"; return;
  case AVM::FLAGS: OS << "flags"; return;
  case AVM::PB: OS << "pb"; return;
  case AVM::CB: OS << "cb"; return;
  default: OS << "<bad-reg>"; return;
  }
}

void AVMInstPrinter::printCompactReg(MCRegister Reg, raw_ostream &OS) const {
  switch (Reg.id()) {
  case AVM::R4: OS << "c0"; return;
  case AVM::R5: OS << "c1"; return;
  case AVM::R6: OS << "c2"; return;
  case AVM::R7: OS << "c3"; return;
  default: printFullReg(Reg, OS); return;
  }
}

void AVMInstPrinter::printPairReg(MCRegister Reg, raw_ostream &OS) const {
  switch (Reg.id()) {
  case AVM::R0R1: OS << "q0"; return;
  case AVM::R2R3: OS << "q1"; return;
  case AVM::R4R5: OS << "q2"; return;
  case AVM::R6R7: OS << "q3"; return;
  default: OS << "<bad-pair-reg>"; return;
  }
}

void AVMInstPrinter::printRegName(raw_ostream &OS, MCRegister Reg) {
  printFullReg(Reg, OS);
}

void AVMInstPrinter::printOperand(const MCOperand &Op, raw_ostream &OS) const {
  if (Op.isReg()) {
    printFullReg(Op.getReg(), OS);
    return;
  }
  if (Op.isImm()) {
    OS << formatImm(Op.getImm());
    return;
  }
  if (Op.isExpr()) {
    MAI.printExpr(OS, *Op.getExpr());
    return;
  }
  llvm_unreachable("unsupported AVM operand");
}

void AVMInstPrinter::printInst(const MCInst *MI, uint64_t, StringRef Annot,
                               const MCSubtargetInfo &, raw_ostream &OS) {
  OS << getMnemonic(*MI).first;
  auto Op = [&](unsigned I) { printOperand(MI->getOperand(I), OS); };
  auto FullReg = [&](unsigned I) { printFullReg(MI->getOperand(I).getReg(), OS); };
  auto CompactReg = [&](unsigned I) { printCompactReg(MI->getOperand(I).getReg(), OS); };
  auto PairReg = [&](unsigned I) { printPairReg(MI->getOperand(I).getReg(), OS); };
  auto SignedSuffix = [&](unsigned I) {
    const MCOperand &O = MI->getOperand(I);
    if (O.isImm() && O.getImm() < 0) {
      OS << O.getImm();
    } else {
      OS << '+';
      printOperand(O, OS);
    }
  };

  switch (MI->getOpcode()) {
  case AVM::NOP:
  case AVM::RET:
    break;
  case AVM::CLR:
  case AVM::TST16:
  case AVM::TST8:
    OS << '\t'; CompactReg(0); break;
  case AVM::MOVC:
  case AVM::ADDC:
  case AVM::SUBC:
  case AVM::CMP16C:
  case AVM::CMP8C:
  case AVM::ADDNF:
  case AVM::SUBNF:
    OS << '\t'; CompactReg(0); OS << ", "; CompactReg(1); break;
  case AVM::MOV32:
  case AVM::ADD32:
  case AVM::SUB32:
  case AVM::AND32:
  case AVM::OR32:
  case AVM::XOR32:
  case AVM::CMP32:
  case AVM::SHL32V:
  case AVM::LSR32V:
  case AVM::ASR32V:
    OS << '\t'; PairReg(0); OS << ", "; PairReg(1); break;
  case AVM::AND16:
  case AVM::OR16:
  case AVM::XOR16:
  case AVM::BIC16:
    OS << '\t'; CompactReg(0); OS << ", "; CompactReg(1); break;
  case AVM::ANDA:
  case AVM::ORA:
  case AVM::XORA:
  case AVM::BICA:
    OS << '\t'; CompactReg(0); OS << ", "; FullReg(1); break;
  case AVM::LD8C:
  case AVM::LD16C:
    OS << '\t'; CompactReg(0); OS << ", ["; CompactReg(1); OS << ']'; break;
  case AVM::ST8C:
  case AVM::ST16C:
    OS << "\t["; CompactReg(0); OS << "], "; CompactReg(1); break;
  case AVM::LDI8C:
    OS << '\t'; CompactReg(0); OS << ", "; Op(1); break;
  case AVM::LD8:
  case AVM::LD16:
    OS << '\t'; FullReg(0); OS << ", ["; FullReg(1); OS << ']'; break;
  case AVM::ST8:
  case AVM::ST16:
    OS << "\t["; FullReg(0); OS << "], "; FullReg(1); break;
  case AVM::LD8_POST:
  case AVM::LD16_POST:
    OS << '\t'; FullReg(0); OS << ", ["; FullReg(1); OS << "]+"; break;
  case AVM::ST8_POST:
  case AVM::ST16_POST:
    OS << "\t["; FullReg(0); OS << "]+, "; FullReg(1); break;
  case AVM::LEA:
  case AVM::LD8_DISP:
  case AVM::LD16_DISP:
    OS << '\t'; FullReg(0); OS << ", ["; FullReg(1); SignedSuffix(2); OS << ']'; break;
  case AVM::ST8_DISP:
  case AVM::ST16_DISP:
    OS << "\t["; FullReg(0); SignedSuffix(1); OS << "], "; FullReg(2); break;
  case AVM::LDSP8C:
  case AVM::LDSP16C:
    OS << '\t'; CompactReg(0); OS << ", [sp+"; Op(1); OS << ']'; break;
  case AVM::STSP8C:
  case AVM::STSP16C:
    OS << "\t[sp+"; Op(0); OS << "], "; CompactReg(1); break;
  case AVM::LDSP8:
  case AVM::LDSP16:
    OS << '\t'; FullReg(0); OS << ", [sp+"; Op(1); OS << ']'; break;
  case AVM::STSP8:
  case AVM::STSP16:
    OS << "\t[sp+"; Op(0); OS << "], "; FullReg(1); break;
  case AVM::LDM8:
  case AVM::LDM16:
    OS << '\t'; FullReg(0); OS << ", "; Op(1); break;
  case AVM::STM8:
  case AVM::STM16:
    OS << '\t'; Op(0); OS << ", "; FullReg(1); break;
  case AVM::LDP8:
  case AVM::LDP16:
    OS << '\t'; FullReg(0); OS << ", [pb:"; FullReg(1); OS << ']'; break;
  case AVM::LDP8_DISP:
  case AVM::LDP16_DISP:
    OS << '\t'; FullReg(0); OS << ", [pb:"; FullReg(1); SignedSuffix(2); OS << ']'; break;
  case AVM::MOV16:
  case AVM::MOV16_E3:
  case AVM::MOV8Z:
  case AVM::MOV8S:
  case AVM::ADD16:
  case AVM::SUB16:
  case AVM::CMP16:
  case AVM::ADC16:
  case AVM::SBC16:
  case AVM::CMP8:
  case AVM::CPC16:
  case AVM::MULU8:
  case AVM::MULS8:
  case AVM::MULSU8:
  case AVM::SHL16V:
  case AVM::LSR16V:
  case AVM::ASR16V:
    OS << '\t'; FullReg(0); OS << ", "; FullReg(1); break;
  case AVM::CSET: {
    static const char *Conditions[] = {"eq", "ne", "ult", "uge",
                                       "slt", "sge", "ule", "ugt"};
    OS << '\t'; FullReg(0); OS << ", ";
    int64_t CC = MI->getOperand(1).getImm();
    if (CC >= 0 && CC < 8) OS << Conditions[CC];
    else Op(1);
    break;
  }
  case AVM::LDI16:
  case AVM::LDI8:
  case AVM::ADDI16:
  case AVM::SUBI16:
  case AVM::ANDI16:
  case AVM::ORI16:
  case AVM::XORI16:
  case AVM::CMPI16:
  case AVM::CMPI8:
    OS << '\t'; FullReg(0); OS << ", "; Op(1); break;
  case AVM::CMPI6:
    OS << '\t'; CompactReg(0); OS << ", "; Op(1); break;
  case AVM::PUSH16:
  case AVM::POP16:
  case AVM::INC16:
  case AVM::DEC16:
  case AVM::NOT16:
  case AVM::NEG16:
  case AVM::LSL16:
  case AVM::LSR16:
  case AVM::ASR16:
  case AVM::LSR8:
  case AVM::ASR8:
  case AVM::ZEXT8:
  case AVM::SEXT8:
  case AVM::SWAP8:
  case AVM::GETSP:
  case AVM::SETSP:
  case AVM::JMPR:
  case AVM::CALLR:
  case AVM::JMPP:
  case AVM::CALLP:
  case AVM::MTPB:
  case AVM::MFPB:
    OS << '\t'; FullReg(0); break;
  default:
    if (MI->getNumOperands()) {
      OS << '\t';
      for (unsigned I = 0; I != MI->getNumOperands(); ++I) {
        if (I) OS << ", ";
        Op(I);
      }
    }
    break;
  }

  printAnnotation(OS, Annot);
}
