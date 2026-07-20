//===--- AVM.h - Declare AVM target feature support ------------*- C++ -*-===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_AVM_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_AVM_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/ADT/StringRef.h"
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
    HasFloat16 = false;
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
    MaxAtomicPromoteWidth = MaxAtomicInlineWidth = 32;
    resetDataLayout("e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-"
                    "n8:16-S8-P1-G0-A0");
  }

  bool isValidCPUName(StringRef Name) const override {
    return Name == "avm1" || Name == "generic";
  }

  void fillValidCPUList(SmallVectorImpl<StringRef> &Values) const override {
    Values.push_back("avm1");
    Values.push_back("generic");
  }

  bool setCPU(const std::string &Name) override { return isValidCPUName(Name); }

  bool isValidTuneCPUName(StringRef Name) const override {
    return Name == "avm-interpreter-32u4-v1";
  }

  void fillValidTuneCPUList(SmallVectorImpl<StringRef> &Values) const override {
    Values.push_back("avm-interpreter-32u4-v1");
  }

  uint64_t getFunctionPointerWidth() const override { return 24; }

  uint64_t getFunctionPointerAlign() const override { return 8; }

  uint64_t getPointerWidthV(LangAS AS) const override {
    return getTargetAddressSpace(AS) == 1 ? 24 : 16;
  }

  uint64_t getPointerAlignV(LangAS) const override { return 8; }

  bool isAddressSpaceSupersetOf(LangAS A, LangAS B) const override {
    auto IsData = [](LangAS AS) {
      return AS == LangAS::Default ||
             (isTargetAddressSpace(AS) && toTargetAddressSpace(AS) == 0);
    };
    return A == B || (IsData(A) && IsData(B));
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override;

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  std::string_view getClobbers() const override { return ""; }

  ArrayRef<const char *> getGCCRegNames() const override {
    static const char *const Names[] = {"r0", "r1", "r2", "r3", "r4",
                                        "r5", "r6", "r7", "q0", "q1",
                                        "q2", "q3", "cc"};
    return Names;
  }

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override {
    return {};
  }

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    if (*Name == '{') {
      StringRef Constraint(Name);
      size_t End = Constraint.find('}');
      if (End == StringRef::npos)
        return false;
      StringRef Register = Constraint.slice(1, End);
      bool Valid =
          Register.size() == 2 &&
          ((Register[0] == 'r' && Register[1] >= '0' && Register[1] <= '7') ||
           (Register[0] == 'q' && Register[1] >= '0' && Register[1] <= '3'));
      if (!Valid)
        return false;
      Name += End;
      Info.setAllowsRegister();
      return true;
    }

    switch (*Name) {
    default:
      return false;
    case 'r':
    case 'c':
    case 'b':
    case 'B':
    case 'p':
    case 'P':
    case 'q':
    case 'Q':
    case 't':
      Info.setAllowsRegister();
      return true;
    case 'I':
      Info.setRequiresImmediate(-128, 127);
      return true;
    case 'J':
      Info.setRequiresImmediate(0, 255);
      return true;
    case 'K':
    case 'N':
      Info.setRequiresImmediate(0, 15);
      return true;
    case 'L':
      Info.setRequiresImmediate(-32768, 32767);
      return true;
    case 'M':
      // ConstraintInfo's signed range cannot express every value of the
      // target's unsigned 16-bit int. The backend enforces the exact range.
      Info.setRequiresImmediate();
      return true;
    case 'O':
      Info.setRequiresImmediate(0);
      return true;
    case 'm':
    case 'o':
      Info.setAllowsMemory();
      return true;
    }
  }

  bool allowsLargerPreferedTypeAlignment() const override { return false; }
};

} // namespace targets
} // namespace clang

#endif
