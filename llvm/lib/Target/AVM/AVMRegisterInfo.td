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

// Architectural 16-bit registers.  Lower registers require the slower full
// encodings for common scalar operations, so bias frequently used values
// toward R4-R7 without making R0-R3 unavailable.
let CostPerUse = [1] in {
def R0 : AVMReg<0, "r0">;
def R1 : AVMReg<1, "r1">;
def R2 : AVMReg<2, "r2">;
def R3 : AVMReg<3, "r3">;
}
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

// Allocation order favors the compact upper-register encodings.  GPR8 names
// the low byte of the same physical registers; it does not create byte
// registers with independent allocation or storage.
def GPR16 : RegisterClass<"AVM", [i16], 8,
                          (add R4, R5, R6, R7, R0, R1, R2, R3)>;
def UpperGPR16 : RegisterClass<"AVM", [i16], 8, (add R4, R5, R6, R7)>;
def LowGPR16 : RegisterClass<"AVM", [i16], 8, (add R0, R1, R2, R3)>;
def PTR16 : RegisterClass<"AVM", [i16], 8,
                          (add R4, R5, R6, R7, R0, R1, R2, R3)>;
def UpperPTR16 : RegisterClass<"AVM", [i16], 8, (add R4, R5, R6, R7)>;
def GPR8 : RegisterClass<"AVM", [i16, i8], 8,
                          (add R4, R5, R6, R7, R0, R1, R2, R3)>;
def UpperGPR8 : RegisterClass<"AVM", [i16, i8], 8, (add R4, R5, R6, R7)>;
def GPR32 : RegisterClass<"AVM", [i32, f32], 8,
                          (add R4R5, R6R7, R0R1, R2R3)>;
def UpperGPR32 : RegisterClass<"AVM", [i32, f32], 8, (add R4R5, R6R7)>;
// A distinct class with the same physical members lets SelectionDAG preserve
// the semantic `t` inline-assembly constraint through register assignment.
def ProgPtrGPR32 : RegisterClass<"AVM", [i32], 8,
                                 (add R4R5, R6R7, R0R1, R2R3)>;

// Singleton classes are pre-RA fixed-service carriers and operands of other
// genuinely fixed target operations. System-service pseudos themselves use
// semantic general classes; the descriptor-driven service passes introduce
// these carriers only where keeping a value in its ABI register is profitable.
def R0Only : RegisterClass<"AVM", [i16], 8, (add R0)>;
def R1Only : RegisterClass<"AVM", [i16], 8, (add R1)>;
def R2Only : RegisterClass<"AVM", [i16], 8, (add R2)>;
def R3Only : RegisterClass<"AVM", [i16], 8, (add R3)>;
def R4Only : RegisterClass<"AVM", [i16], 8, (add R4)>;
def R5Only : RegisterClass<"AVM", [i16], 8, (add R5)>;
def R6Only : RegisterClass<"AVM", [i16], 8, (add R6)>;
def R7Only : RegisterClass<"AVM", [i16], 8, (add R7)>;
def Q0Only : RegisterClass<"AVM", [i32, f32], 8, (add R0R1)>;
def Q1Only : RegisterClass<"AVM", [i32, f32], 8, (add R2R3)>;
def Q2Only : RegisterClass<"AVM", [i32, f32], 8, (add R4R5)>;
def Q3Only : RegisterClass<"AVM", [i32, f32], 8, (add R6R7)>;

// Keep the MC-facing class names so the parser, emitter, printer, and their
// established tests retain their spelling and register acceptance.
def AVMGeneralReg : RegisterClass<"AVM", [i16], 16,
  (add R4, R5, R6, R7, R0, R1, R2, R3)>;
def AVMColdRegClass : RegisterClass<"AVM", [i16], 16, (add R0, R1, R2, R3)>;
def AVMLowRegClass : RegisterClass<"AVM", [i16], 16, (add R0, R1, R2, R3)>;
def AVMCompactReg : RegisterClass<"AVM", [i16], 16, (add R4, R5, R6, R7)>;
def AVMProgramPairReg : RegisterClass<"AVM", [i32, f32], 32,
                                      (add R4R5, R6R7, R0R1, R2R3)>;

def CSR_AVM : CalleeSavedRegs<(add R0, R1, R2, R3)>;
def CSR_AVM_CALL : CalleeSavedRegs<(add R0, R1, R2, R3, SP)>;
