//===- ABCTargetTransformInfo.h - ABC-specific TTI ------------------------===//

#ifndef LLVM_LIB_TARGET_ABC_ABCTARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_ABC_ABCTARGETTRANSFORMINFO_H

#include "ABCSubtarget.h"
#include "ABCTargetMachine.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/IR/Function.h"

namespace llvm {

class ABCTTIImpl final : public BasicTTIImplBase<ABCTTIImpl> {
  using BaseT = BasicTTIImplBase<ABCTTIImpl>;

  friend BaseT;

  const ABCSubtarget *ST;
  const ABCTargetLowering *TLI;

  const ABCSubtarget *getST() const { return ST; }
  const ABCTargetLowering *getTLI() const { return TLI; }

public:
  explicit ABCTTIImpl(const ABCTargetMachine *TM, const Function &F)
      : BaseT(TM, F.getDataLayout()), ST(TM->getSubtargetImpl(F)),
        TLI(ST->getTargetLowering()) {}

  TypeSize
  getRegisterBitWidth(TargetTransformInfo::RegisterKind K) const override {
    switch (K) {
    case TargetTransformInfo::RGK_Scalar:
      return TypeSize::getFixed(32);
    case TargetTransformInfo::RGK_FixedWidthVector:
      return TypeSize::getFixed(0);
    case TargetTransformInfo::RGK_ScalableVector:
      return TypeSize::getScalable(0);
    }
    llvm_unreachable("Unsupported register kind");
  }

  bool isLegalToVectorizeLoad(LoadInst *) const override { return false; }
  bool isLegalToVectorizeStore(StoreInst *) const override { return false; }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_ABCTARGETTRANSFORMINFO_H
