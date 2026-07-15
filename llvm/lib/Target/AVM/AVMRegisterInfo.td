//===-- AVMRegisterInfo.td - AVM register definitions -------*- tablegen -*-===//

let Namespace = "AVM" in {
  def sub_lo16 : SubRegIndex<16>;
  def sub_hi16 : SubRegIndex<16, 16>;
}

class AVMReg<bits<16> Enc, string Name, list<Register> SubRegs = []>
    : RegisterWithSubRegs<Name, SubRegs> {
  let Namespace = "AVM";
  let HWEncoding = Enc;
}

// Architectural 16-bit registers.
def R0 : AVMReg<0, "r0">;
def R1 : AVMReg<1, "r1">;
def R2 : AVMReg<2, "r2">;
def R3 : AVMReg<3, "r3">;
def R4 : AVMReg<4, "r4">;
def R5 : AVMReg<5, "r5">;
def R6 : AVMReg<6, "r6">;
def R7 : AVMReg<7, "r7">;

// Canonical aligned pairs.
let SubRegIndices = [sub_lo16, sub_hi16], CoveredBySubRegs = 1 in {
  def R0R1 : AVMReg<0, "r0:r1", [R0, R1]>;
  def R2R3 : AVMReg<2, "r2:r3", [R2, R3]>;
  def R4R5 : AVMReg<4, "r4:r5", [R4, R5]>;
  def R6R7 : AVMReg<6, "r6:r7", [R6, R7]>;
}

def SP : AVMReg<8, "sp">;
def PC : AVMReg<9, "pc">;
def CC : AVMReg<10, "cc">;

// Required by TableGen's MC register metadata.  It has no instruction or
// code-generation semantics and contains only general architectural words.
def AVMGeneralReg : RegisterClass<"AVM", [i16], 16,
                                  (add R0, R1, R2, R3, R4, R5, R6, R7)>;

// c0-c3 are the compact aliases for r4-r7.  This class is MC metadata for
// compact operand validation; it carries no register-allocation semantics.
def AVMCompactReg : RegisterClass<"AVM", [i16], 16, (add R4, R5, R6, R7)>;

// Aligned 32-bit register pairs are MC-only program pointers.  Their
// architectural encoding is deliberately supplied by the code emitter rather
// than inferred from physical-register enum values.
def AVMProgramPairReg : RegisterClass<"AVM", [i32], 32,
                                      (add R0R1, R2R3, R4R5, R6R7)>;
