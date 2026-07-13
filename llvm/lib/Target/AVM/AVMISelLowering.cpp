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
  setMinFunctionAlignment(Align(1));
  computeRegisterProperties(STI.getRegisterInfo());
}
