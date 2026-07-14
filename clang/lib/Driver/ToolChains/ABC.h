//===--- ABC.h - ABC-specific Tool Helpers ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ABC_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ABC_H

#include "Gnu.h"
#include "clang/Driver/Tool.h"
#include "clang/Driver/ToolChain.h"

namespace clang {
namespace driver {
namespace toolchains {

class LLVM_LIBRARY_VISIBILITY ABCToolChain : public Generic_ELF {
public:
  ABCToolChain(const Driver &D, const llvm::Triple &Triple,
               const llvm::opt::ArgList &Args)
      : Generic_ELF(D, Triple, Args) {}

  bool isBareMetal() const override { return true; }
  bool isCrossCompiling() const override { return true; }
  bool HasNativeLLVMSupport() const override { return true; }
  bool isPICDefault() const override { return false; }
  bool isPIEDefault(const llvm::opt::ArgList &Args) const override {
    return false;
  }
  bool isPICDefaultForced() const override { return false; }
  bool SupportsProfiling() const override { return false; }
  StringRef getOSLibName() const override { return "abc"; }

  RuntimeLibType GetDefaultRuntimeLibType() const override {
    return ToolChain::RLT_CompilerRT;
  }

  UnwindLibType GetUnwindLibType(const llvm::opt::ArgList &Args) const override {
    return ToolChain::UNW_None;
  }

  UnwindTableLevel
  getDefaultUnwindTableLevel(const llvm::opt::ArgList &Args) const override {
    return UnwindTableLevel::None;
  }

  SanitizerMask
  getSupportedSanitizers(BoundArch BA,
                         Action::OffloadKind DeviceOffloadKind) const override;

  void
  AddClangSystemIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                            llvm::opt::ArgStringList &CC1Args) const override;
  void addClangTargetOptions(const llvm::opt::ArgList &DriverArgs,
                             llvm::opt::ArgStringList &CC1Args, BoundArch BA,
                             Action::OffloadKind DeviceOffloadKind) const override;

protected:
  Tool *buildLinker() const override;
};

} // namespace toolchains

namespace tools {
namespace abc {

class LLVM_LIBRARY_VISIBILITY Linker final : public Tool {
public:
  Linker(const ToolChain &TC) : Tool("ABC::Linker", "abc-ld", TC) {}
  bool hasIntegratedCPP() const override { return false; }
  bool isLinkJob() const override { return true; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};

} // namespace abc
} // namespace tools
} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ABC_H
