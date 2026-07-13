//===--- AVM.cpp - Implement AVM target feature support ------------------===//

#include "AVM.h"
#include "clang/Basic/MacroBuilder.h"

using namespace clang;
using namespace clang::targets;

void AVMTargetInfo::getTargetDefines(const LangOptions &,
                                     MacroBuilder &Builder) const {
  Builder.defineMacro("AVM");
  Builder.defineMacro("__AVM");
  Builder.defineMacro("__AVM__");
  Builder.defineMacro("__ARDUBOY__");
  Builder.defineMacro("__ARDUBOY_FX__");
}
