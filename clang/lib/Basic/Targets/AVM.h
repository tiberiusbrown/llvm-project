//===--- AVM.h - Declare AVM target feature support ------------*- C++ -*-===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_AVM_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_AVM_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY AVMTargetInfo final : public TargetInfo {
public:
  AVMTargetInfo(const llvm::Triple &Triple, const TargetOptions &)
      : TargetInfo(Triple) {
    TLSSupported = false;
    PointerWidth = 16;
    PointerAlign = 8;
    BoolWidth = BoolAlign = 8;
    ShortWidth = IntWidth = 16;
    ShortAlign = IntAlign = 8;
    LongWidth = 32;
    LongAlign = 8;
    LongLongWidth = 64;
    LongLongAlign = 8;
    HalfWidth = 16;
    HalfAlign = 8;
    HasFloat16 = true;
    FloatWidth = DoubleWidth = LongDoubleWidth = 32;
    FloatAlign = DoubleAlign = LongDoubleAlign = 8;
    DoubleFormat = LongDoubleFormat = &llvm::APFloat::IEEEsingle();
    SuitableAlign = DefaultAlignForAttributeAligned = 8;

    SizeType = UnsignedInt;
    PtrDiffType = SignedInt;
    IntPtrType = SignedInt;
    WCharType = UnsignedInt;
    WIntType = SignedInt;
    Char16Type = UnsignedInt;
    Char32Type = UnsignedLong;
    Int16Type = SignedInt;
    SigAtomicType = SignedChar;
    resetDataLayout();
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override {
    return {};
  }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  std::string_view getClobbers() const override { return ""; }

  ArrayRef<const char *> getGCCRegNames() const override {
    static const char *const Names[] = {"r0", "r1", "r2", "r3",    "r4", "r5",
                                        "r6", "r7", "sp", "flags", "pb", "cb"};
    return Names;
  }

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override {
    return {};
  }

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    if (*Name == 'r') {
      Info.setAllowsRegister();
      return true;
    }
    return false;
  }

  bool allowsLargerPreferedTypeAlignment() const override { return false; }
};

} // namespace targets
} // namespace clang

#endif
