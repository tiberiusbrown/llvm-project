//===--- AVM.cpp - Implement AVM target feature support ------------------===//

#include "AVM.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetBuiltins.h"

using namespace clang;
using namespace clang::targets;

static constexpr int NumBuiltins = AVM::LastTSBuiltin - Builtin::FirstTSBuiltin;

static constexpr llvm::StringTable BuiltinStrings =
    CLANG_BUILTIN_STR_TABLE_START
#define BUILTIN CLANG_BUILTIN_STR_TABLE
#include "clang/Basic/BuiltinsAVM.def"
    ;

static constexpr auto BuiltinInfos = Builtin::MakeInfos<NumBuiltins>({
#define BUILTIN CLANG_BUILTIN_ENTRY
#define LIBBUILTIN CLANG_LIBBUILTIN_ENTRY
#include "clang/Basic/BuiltinsAVM.def"
});

void AVMTargetInfo::getTargetDefines(const LangOptions &,
                                     MacroBuilder &Builder) const {
  Builder.defineMacro("AVM");
  Builder.defineMacro("__AVM");
  Builder.defineMacro("__AVM__");
  Builder.defineMacro("__ARDUBOY__");
  Builder.defineMacro("__ARDUBOY_FX__");
}

llvm::SmallVector<Builtin::InfosShard>
AVMTargetInfo::getTargetBuiltins() const {
  return {{&BuiltinStrings, BuiltinInfos}};
}
