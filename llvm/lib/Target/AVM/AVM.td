//===-- AVM.td - Top-level AVM target definition -----------*- tablegen -*-===//

include "llvm/Target/Target.td"
include "AVMRegisterInfo.td"
include "AVMInstrInfo.td"

// Target.td requires this remapping while generating MC instruction metadata.
// It does not register a code generator or make the pseudos selectable.
defm : RemapAllTargetPseudoPointerOperands<GPR16>;

def FeatureV1 : SubtargetFeature<"v1", "IsV1", "true",
                                  "AVM Version 1 ISA">;
def : Processor<"generic", NoItineraries, [FeatureV1]>;

def AVMInstrInfo : InstrInfo;
def AVM : Target {
  let InstructionSet = AVMInstrInfo;
}
