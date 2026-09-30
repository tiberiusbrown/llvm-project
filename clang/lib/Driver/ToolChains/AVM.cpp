//===--- AVM.cpp - AVM SDK toolchain ---------------------------------------===//
#include "AVM.h"
#include "clang/Driver/CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/InputInfo.h"
#include "clang/Options/Options.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace llvm::opt;

AVM::AVM(const Driver &D, const llvm::Triple &Triple, const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  SmallString<128> Lib(computeSysRoot());
  llvm::sys::path::append(Lib, "lib");
  getFilePaths().push_back(std::string(Lib));
}

std::string AVM::computeSysRoot() const {
  if (!getDriver().SysRoot.empty())
    return getDriver().SysRoot;

  // Installed SDK: <prefix>/bin and <prefix>/sysroot.
  SmallString<128> SDK(getDriver().Dir);
  llvm::sys::path::append(SDK, "..", "sysroot");
  if (llvm::sys::fs::exists(SDK))
    return std::string(SDK);

  // Build tree: <build>/llvm-build/bin and <build>/avm-sysroot.
  SmallString<128> Build(getDriver().Dir);
  llvm::sys::path::append(Build, "..", "..", "avm-sysroot");
  return std::string(Build);
}

void AVM::AddClangSystemIncludeArgs(const ArgList &Args,
                                    ArgStringList &CC1Args) const {
  if (Args.hasArg(options::OPT_nostdinc, options::OPT_nostdlibinc))
    return;
  SmallString<128> Include(computeSysRoot());
  llvm::sys::path::append(Include, "include");
  addSystemInclude(Args, CC1Args, Include);
}

void AVM::addClangTargetOptions(const ArgList &, ArgStringList &CC1Args,
                                Action::OffloadKind) const {
  // The SDK has its own headers; host headers must never leak into a build.
  CC1Args.push_back("-nostdsysteminc");
}

DerivedArgList *AVM::TranslateArgs(const DerivedArgList &Args, StringRef,
                                   Action::OffloadKind) const {
  auto *DAL = new DerivedArgList(Args.getBaseArgs());
  const OptTable &Opts = getDriver().getOpts();
  auto Add = [&](options::ID ID) { DAL->AddFlagArg(nullptr, Opts.getOption(ID)); };

  // Put defaults first so explicit command-line options take precedence.
  Add(options::OPT_ffreestanding);
  Add(options::OPT_fomit_frame_pointer);
  Add(options::OPT_fno_stack_protector);
  Add(options::OPT_fno_unwind_tables);
  Add(options::OPT_fno_asynchronous_unwind_tables);
  Add(options::OPT_ffunction_sections);
  Add(options::OPT_fdata_sections);
  if (!Args.hasArg(options::OPT_O_Group))
    DAL->AddJoinedArg(nullptr, Opts.getOption(options::OPT_O), "2");
  Add(options::OPT_fno_exceptions);
  Add(options::OPT_fno_rtti);
  Add(options::OPT_fno_threadsafe_statics);
  Add(options::OPT_fno_use_cxa_atexit);
  for (Arg *A : Args)
    DAL->append(A);
  return DAL;
}

Tool *AVM::buildLinker() const { return new tools::avm::Linker(*this); }

void tools::avm::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                                      const InputInfo &Output,
                                      const InputInfoList &Inputs,
                                      const ArgList &Args,
                                      const char *) const {
  const ToolChain &TC = getToolChain();
  ArgStringList CmdArgs;
  CmdArgs.push_back("-flavor");
  CmdArgs.push_back("gnu");
  CmdArgs.push_back(Args.MakeArgString(
      "--entry=" + Args.getLastArgValue(options::OPT_avm_entry_EQ, "_start")));
  CmdArgs.push_back("--gc-sections");
  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  StringRef Startup = Args.getLastArgValue(options::OPT_avm_startup_EQ,
                                           "crt0.o");
  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles) &&
      Startup != "none") {
    std::string StartupPath;
    if (Startup == "crt0.o" || Startup == "crt0_test.o" ||
        Startup == "crt0_sketch.o")
      StartupPath = TC.GetFilePath(Startup.str().c_str());
    else
      StartupPath = Startup.str();
    CmdArgs.push_back(Args.MakeArgString(StartupPath));
  }

  Args.addAllArgs(CmdArgs, {options::OPT_L, options::OPT_u, options::OPT_T});
  AddLinkerInputs(TC, Inputs, Args, CmdArgs, JA);
  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs)) {
    SmallString<128> Lib(TC.computeSysRoot());
    llvm::sys::path::append(Lib, "lib");
    CmdArgs.push_back(Args.MakeArgString("-L" + Lib));
    CmdArgs.push_back("-lavm");
    CmdArgs.push_back("-lavm-builtins");
  }

  C.addCommand(std::make_unique<Command>(
      JA, *this, ResponseFileSupport::AtFileCurCP(),
      Args.MakeArgString(TC.GetProgramPath("avm-ld")), CmdArgs, Inputs, Output));

  if (Args.hasArg(options::OPT_avm_no_image))
    return;

  SmallString<128> Image(Args.getLastArgValue(options::OPT_avm_image_EQ));
  if (Image.empty()) {
    Image = Output.getFilename();
    llvm::sys::path::replace_extension(Image, "bin");
  }
  ArgStringList ImageArgs;
  ImageArgs.push_back("--development");
  ImageArgs.push_back(Output.getFilename());
  ImageArgs.push_back("-o");
  ImageArgs.push_back(Args.MakeArgString(Image));
  C.addCommand(std::make_unique<Command>(
      JA, *this, ResponseFileSupport::AtFileCurCP(),
      Args.MakeArgString(TC.GetProgramPath("avm-image")), ImageArgs,
      Inputs, Output));
}
