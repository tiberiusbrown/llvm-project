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
  unsigned DataSourceArg;
  switch (BuiltinID) {
  default:
    return false;
  case AVM::BI__builtin_avm_stack_dispatch: {
    if (checkArgCount(TheCall, 2))
      return true;
    Expr *Arg = TheCall->getArg(0);
    auto Converted = DefaultFunctionArrayLvalueConversion(Arg);
    if (Converted.isInvalid())
      return true;
    Arg = Converted.get();
    TheCall->setArg(0, Arg);
    const auto *PT = Arg->getType()->getAs<PointerType>();
    const auto *FT = PT ? PT->getPointeeType()->getAs<FunctionProtoType>() : nullptr;
    Expr::EvalResult Kind;
    if (!FT || !FT->getReturnType()->isVoidType() || FT->getNumParams() ||
        FT->isVariadic() || !TheCall->getArg(1)->EvaluateAsInt(Kind, Context) ||
        Kind.Val.getInt().isNegative() || Kind.Val.getInt().ugt(3)) {
      Diag(TheCall->getExprLoc(), diag::err_avm_stack_dispatch);
      return true;
    }
    return false;
  }
  case AVM::BI__builtin_avm_flash_string: {
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

  case AVM::BI__builtin_avm_debug_printf_p:
  case AVM::BI__avm_debug_printf_P:
  case AVM::BI__builtin_avm_snprintf:
  case AVM::BI__avm_snprintf:
  case AVM::BI__builtin_avm_snprintf_p:
  case AVM::BI__avm_snprintf_P:
  case AVM::BI__builtin_avm_draw_textf:
  case AVM::BI__avm_draw_textf:
  case AVM::BI__builtin_avm_draw_textf_p:
  case AVM::BI__avm_draw_textf_P: {
    bool Invalid = false;
    unsigned FirstVariadicArg = 3;
    if (BuiltinID == AVM::BI__builtin_avm_debug_printf_p ||
        BuiltinID == AVM::BI__avm_debug_printf_P)
      FirstVariadicArg = 1;

    for (unsigned I = FirstVariadicArg; I != TheCall->getNumArgs(); ++I) {
      Expr *Arg = TheCall->getArg(I);
      if (Arg->isTypeDependent())
        continue;
      QualType Ty = Arg->getType();
      if (Ty->isIntegerType() || Ty->isEnumeralType() ||
          Ty->isRealFloatingType() || Ty->isPointerType() ||
          Ty->isNullPtrType())
        continue;
      Diag(Arg->getExprLoc(), diag::err_avm_variadic_builtin_arg)
          << I + 1 << Ty << Arg->getSourceRange();
      Invalid = true;
    }
    return Invalid;
  }

  case AVM::BI__avm_memcmp:
  case AVM::BI__avm_strcmp:
  case AVM::BI__avm_strncpy:
  case AVM::BI__avm_strncat:
    DataSourceArg = 1;
    break;
  case AVM::BI__avm_strlen:
    DataSourceArg = 0;
    break;
  }

  if (TheCall->getNumArgs() <= DataSourceArg)
    return false;

  Expr *Arg = TheCall->getArg(DataSourceArg)->IgnoreParenImpCasts();
  QualType ArgType = Arg->getType();
  const auto *ArgPointer = ArgType->getAs<PointerType>();
  if (!ArgPointer ||
      ArgPointer->getPointeeType().getAddressSpace() == LangAS::Default)
    return false;

  const FunctionDecl *Callee = TheCall->getDirectCallee();
  assert(Callee && Callee->getNumParams() > DataSourceArg &&
         "AVM builtin must have a direct declaration");
  QualType ParamType = Callee->getParamDecl(DataSourceArg)->getType();
  Diag(Arg->getExprLoc(), diag::err_typecheck_incompatible_address_space)
      << ArgType << ParamType << AssignmentAction::Passing
      << Arg->getSourceRange();
  return true;
}
