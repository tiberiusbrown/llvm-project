#include "AVMISelLowering.h"
#include "AVMSubtarget.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"

using namespace llvm;

AVMTargetLowering::AVMTargetLowering(const TargetMachine &TM,
                                     const AVMSubtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i16, &AVM::CGPR16RegClass);
  addRegisterClass(MVT::i8, &AVM::CGPR8RegClass);
  setStackPointerRegisterToSaveRestore(AVM::SP);
  setBooleanContents(ZeroOrOneBooleanContent);
  // Far-control relocations require bit zero for the link bit.  Function
  // symbols therefore have to be even before final layout is known.
  setMinFunctionAlignment(Align(2));
  computeRegisterProperties(STI.getRegisterInfo());
}
