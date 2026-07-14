//===-- ABC.td - Describe the ABC VM target machine --------*- tablegen -*-===//

include "llvm/Target/Target.td"
include "ABCStubs.td"

def : ProcessorModel<"generic", NoSchedModel, []>;

defm : RemapAllTargetPseudoPointerOperands<STACK>;

def ABCInstrInfo : InstrInfo;

def ABC : Target {
  let InstructionSet = ABCInstrInfo;
}
