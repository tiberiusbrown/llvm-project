//===--- AVM.h - AVM SDK toolchain -------------------------------*- C++ -*-===//
#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_AVM_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_AVM_H

#include "clang/Driver/Tool.h"
#include "clang/Driver/ToolChain.h"

namespace clang::driver {
namespace toolchains {

class LLVM_LIBRARY_VISIBILITY AVM : public ToolChain {
public:
  AVM(const Driver &D, const llvm::Triple &Triple,
      const llvm::opt::ArgList &Args);

  bool isCrossCompiling() const override { return true; }
  bool isBareMetal() const override { return true; }
  bool HasNativeLLVMSupport() const override { return true; }
  bool isPICDefault() const override { return false; }
  bool isPIEDefault(const llvm::opt::ArgList &) const override { return false; }
  bool isPICDefaultForced() const override { return false; }
  bool SupportsProfiling() const override { return false; }
  UnwindTableLevel getDefaultUnwindTableLevel(
      const llvm::opt::ArgList &) const override {
    return UnwindTableLevel::None;
  }

  std::string computeSysRoot() const override;
  void AddClangSystemIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                                 llvm::opt::ArgStringList &CC1Args) const override;
  void addClangTargetOptions(const llvm::opt::ArgList &DriverArgs,
                             llvm::opt::ArgStringList &CC1Args,
                             Action::OffloadKind) const override;
  llvm::opt::DerivedArgList *
  TranslateArgs(const llvm::opt::DerivedArgList &Args, StringRef BoundArch,
                Action::OffloadKind DeviceOffloadKind) const override;

protected:
  Tool *buildLinker() const override;
};

} // namespace toolchains

namespace tools::avm {
class LLVM_LIBRARY_VISIBILITY Linker final : public Tool {
public:
  Linker(const ToolChain &TC) : Tool("AVM::Linker", "lld", TC) {}
  bool hasIntegratedCPP() const override { return false; }
  bool isLinkJob() const override { return true; }
  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &Args,
                    const char *LinkingOutput) const override;
};
} // namespace tools::avm
} // namespace clang::driver

#endif
