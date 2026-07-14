//===--- ABC.cpp - Implement ABC target feature support -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABC.h"
#include "clang/Basic/MacroBuilder.h"

using namespace clang;
using namespace clang::targets;

const char *const ABCTargetInfo::GCCRegNames[] = {"stk"};

ArrayRef<const char *> ABCTargetInfo::getGCCRegNames() const {
  return llvm::ArrayRef(GCCRegNames);
}

void ABCTargetInfo::getTargetDefines(const LangOptions &Opts,
                                     MacroBuilder &Builder) const {
  Builder.defineMacro("__ABC__");
  Builder.defineMacro("__abc__");
  Builder.defineMacro("__ABC_VM__");
  Builder.defineMacro("__ABC_FREESTANDING__");
  Builder.defineMacro("__prog", "const __attribute__((address_space(1)))");
}
