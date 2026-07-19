//===-- AVM.td - Top-level AVM target definition -----------*- tablegen -*-===//

include "llvm/Target/Target.td"
include "AVMRegisterInfo.td"
include "AVMInstrInfo.td"

// Target.td requires a pointer-like register remapping while it generates MC
// metadata.  This creates no AVM source mnemonic, parser rule, or encoding.
defm : RemapAllTargetPseudoPointerOperands<AVMGeneralReg>;

def FeatureV1 : SubtargetFeature<"v1", "IsV1", "true",
                                  "AVM Version 1 ISA">;
include "AVMSchedule.td"
def : Processor<"avm1", NoItineraries, [FeatureV1]>;
def : Processor<"generic", NoItineraries, [FeatureV1]>;

def AVMInstrInfo : InstrInfo;
def AVM : Target {
  let InstructionSet = AVMInstrInfo;
}
