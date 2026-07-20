//===- AVM.cpp ------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "TargetInfo.h"

using namespace clang;
using namespace clang::CodeGen;

namespace {
class AVMABIInfo final : public DefaultABIInfo {
  ABIArgInfo classifyAggregate(QualType Ty, bool IsReturn) const {
    uint64_t Size = getContext().getTypeSizeInChars(Ty).getQuantity();
    if (Size == 0)
      return ABIArgInfo::getIgnore();

    if (Size <= 4) {
      llvm::Type *CoerceTy = llvm::IntegerType::get(getVMContext(), Size * 8);
      return ABIArgInfo::getDirect(CoerceTy);
    }

    return ABIArgInfo::getIndirect(CharUnits::One(),
                                   getDataLayout().getAllocaAddrSpace(),
                                   /*ByVal=*/!IsReturn);
  }

  ABIArgInfo classify(QualType Ty, bool IsReturn) const {
    if (Ty->isVoidType())
      return ABIArgInfo::getIgnore();
    if (isAggregateTypeForABI(Ty))
      return classifyAggregate(Ty, IsReturn);
    if (!IsReturn && isPromotableIntegerTypeForABI(Ty))
      return ABIArgInfo::getExtend(Ty);
    return ABIArgInfo::getDirect();
  }

public:
  explicit AVMABIInfo(CodeGenTypes &CGT) : DefaultABIInfo(CGT) {}

  void computeInfo(CGFunctionInfo &FI) const override {
    if (!getCXXABI().classifyReturnType(FI))
      FI.getReturnInfo() = classify(FI.getReturnType(), /*IsReturn=*/true);
    for (auto &Arg : FI.arguments())
      Arg.info = classify(Arg.type, /*IsReturn=*/false);
  }

  RValue EmitVAArg(CodeGenFunction &CGF, Address VAListAddr, QualType Ty,
                   AggValueSlot Slot) const override {
    bool IsIndirect =
        isAggregateTypeForABI(Ty) && getContext().getTypeSize(Ty) > 32;
    return emitVoidPtrVAArg(CGF, VAListAddr, Ty, IsIndirect,
                            getContext().getTypeInfoInChars(Ty),
                            CharUnits::One(), /*AllowHigherAlign=*/false, Slot);
  }
};

class AVMTargetCodeGenInfo final : public TargetCodeGenInfo {
public:
  explicit AVMTargetCodeGenInfo(CodeGenTypes &CGT)
      : TargetCodeGenInfo(std::make_unique<AVMABIInfo>(CGT)) {}
};
} // namespace

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createAVMTargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<AVMTargetCodeGenInfo>(CGM.getTypes());
}
