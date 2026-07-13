//===-- AVMRegisterInfo.td - AVM register definitions -------*- tablegen -*-===//

let Namespace = "AVM" in {
  def sub_lo8  : SubRegIndex<8>;
  def sub_lo16 : SubRegIndex<16>;
  def sub_hi16 : SubRegIndex<16, 16>;
}

class AVMReg<bits<16> Enc, string Name, list<Register> SubRegs = []>
    : RegisterWithSubRegs<Name, SubRegs> {
  let Namespace = "AVM";
  let HWEncoding = Enc;
}

class AVMByteReg<bits<16> Enc, string Name> : AVMReg<Enc, Name>;
class AVMWordReg<bits<16> Enc, string Name, Register Lo>
    : AVMReg<Enc, Name, [Lo]> {
  let SubRegIndices = [sub_lo8];
  let CoveredBySubRegs = 0;
}

// Low-byte aliases. These are not independent storage.
def B0 : AVMByteReg<0, "b0">;
def B1 : AVMByteReg<1, "b1">;
def B2 : AVMByteReg<2, "b2">;
def B3 : AVMByteReg<3, "b3">;
def B4 : AVMByteReg<4, "b4">;
def B5 : AVMByteReg<5, "b5">;
def B6 : AVMByteReg<6, "b6">;
def B7 : AVMByteReg<7, "b7">;

// Architectural 16-bit registers.
def R0 : AVMWordReg<0, "r0", B0>;
def R1 : AVMWordReg<1, "r1", B1>;
def R2 : AVMWordReg<2, "r2", B2>;
def R3 : AVMWordReg<3, "r3", B3>;
def R4 : AVMWordReg<4, "r4", B4>;
def R5 : AVMWordReg<5, "r5", B5>;
def R6 : AVMWordReg<6, "r6", B6>;
def R7 : AVMWordReg<7, "r7", B7>;

// Even-aligned pairs used for i32 and register-form program pointers.
let SubRegIndices = [sub_lo16, sub_hi16], CoveredBySubRegs = 1 in {
  def R0R1 : AVMReg<0, "r0:r1", [R0, R1]>;
  def R2R3 : AVMReg<2, "r2:r3", [R2, R3]>;
  def R4R5 : AVMReg<4, "r4:r5", [R4, R5]>;
  def R6R7 : AVMReg<6, "r6:r7", [R6, R7]>;
}

// Nonallocatable architectural state.
def SP    : AVMReg<8,  "sp">;
def FLAGS : AVMReg<9,  "flags">;
def PB    : AVMReg<10, "pb">;
def CB    : AVMReg<11, "cb">;

// Allocation order prefers compact caller-saved registers first.
def GPR16  : RegisterClass<"AVM", [i16], 8,
                            (add R4, R5, R6, R7, R0, R1, R2, R3)>;
def CGPR16 : RegisterClass<"AVM", [i16], 8, (add R4, R5, R6, R7)>;
// Registers with dedicated E0 encodings after compact-register redundancies
// have been removed.
def NGPR16 : RegisterClass<"AVM", [i16], 8, (add R0, R1, R2, R3)>;
// Operand classes for the split logical encodings.  The accumulator form has
// a fixed c0/A destination, while the F4 form reserves destination c0/A.
def AGPR16 : RegisterClass<"AVM", [i16], 8, (add R4)>;
def GPR16NoA : RegisterClass<"AVM", [i16], 8,
                               (add R5, R6, R7, R0, R1, R2, R3)>;
def CGPR16NoA : RegisterClass<"AVM", [i16], 8, (add R5, R6, R7)>;
def GPR8   : RegisterClass<"AVM", [i8], 8,
                            (add B4, B5, B6, B7, B0, B1, B2, B3)>;
def CGPR8  : RegisterClass<"AVM", [i8], 8, (add B4, B5, B6, B7)>;
def GPR32  : RegisterClass<"AVM", [i32], 8,
                            (add R4R5, R6R7, R0R1, R2R3)>;

def CSR_AVM : CalleeSavedRegs<(add R0, R1, R2, R3)>;
