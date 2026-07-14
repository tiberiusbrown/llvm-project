//===--- ABC.cpp - ABC-specific Tool Helpers --------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABC.h"
#include "clang/Driver/CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/InputInfo.h"
#include "clang/Options/Options.h"
#include "llvm/Option/ArgList.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang::driver::tools;
using namespace llvm::opt;

SanitizerMask ABCToolChain::getSupportedSanitizers(
    BoundArch BA, Action::OffloadKind DeviceOffloadKind) const {
  return SanitizerMask();
}

void ABCToolChain::AddClangSystemIncludeArgs(const ArgList &DriverArgs,
                                             ArgStringList &CC1Args) const {
  if (DriverArgs.hasArg(options::OPT_nostdinc, options::OPT_nostdlibinc))
    return;

  CC1Args.push_back("-nostdsysteminc");
}

void ABCToolChain::addClangTargetOptions(const ArgList &DriverArgs,
                                         ArgStringList &CC1Args, BoundArch BA,
                                         Action::OffloadKind) const {
  CC1Args.push_back("-nostdsysteminc");
}

Tool *ABCToolChain::buildLinker() const {
  return new tools::abc::Linker(*this);
}

void abc::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                               const InputInfo &Output,
                               const InputInfoList &Inputs,
                               const ArgList &Args,
                               const char *LinkingOutput) const {
  ArgStringList CmdArgs;
  const ToolChain &TC = getToolChain();

  Args.AddAllArgs(CmdArgs, options::OPT_L);
  Args.AddAllArgs(CmdArgs, options::OPT_u);
  Args.AddAllArgs(CmdArgs, options::OPT_s);
  Args.AddAllArgs(CmdArgs, options::OPT_t);
  Args.AddAllArgs(CmdArgs, options::OPT_r);
  AddLinkerInputs(TC, Inputs, Args, CmdArgs, JA);

  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  const char *Exec = Args.MakeArgString(TC.GetProgramPath("abc-ld"));
  C.addCommand(std::make_unique<Command>(JA, *this,
                                         ResponseFileSupport::AtFileCurCP(),
                                         Exec, CmdArgs, Inputs, Output));
}
