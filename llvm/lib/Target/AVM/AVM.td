//===-- AVM.td - Top-level AVM target definition -----------*- tablegen -*-===//

include "llvm/Target/Target.td"
include "AVMRegisterInfo.td"
include "AVMInstrInfo.td"
include "GISel/AVMRegisterBanks.td"

defm : RemapAllTargetPseudoPointerOperands<GPR16>;

def FeatureV1 : SubtargetFeature<"v1", "IsV1", "true",
                                  "AVM Version 1 ISA">;
def : Processor<"generic", NoItineraries, [FeatureV1]>;

def AVMInstrInfo : InstrInfo;
def AVM : Target {
  let InstructionSet = AVMInstrInfo;
}
