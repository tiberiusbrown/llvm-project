//===--- SemaAVM.cpp - AVM target-specific semantic analysis ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Expr.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Basic/TargetBuiltins.h"
#include "clang/Sema/Sema.h"

using namespace clang;

bool Sema::CheckAVMBuiltinFunctionCall(unsigned BuiltinID, CallExpr *TheCall) {
  switch (BuiltinID) {
  default:
    return false;
  case AVM::BI__builtin_avm_flash_string:
    if (checkArgCount(TheCall, 1))
      return true;

    Expr *Arg = TheCall->getArg(0)->IgnoreParenImpCasts();
    if (const auto *Literal = dyn_cast<StringLiteral>(Arg);
        Literal && Literal->getKind() == StringLiteralKind::Ordinary)
      return false;

    Diag(Arg->getExprLoc(), diag::err_avm_flash_string_literal)
        << Arg->getSourceRange();
    return true;
  }
}
