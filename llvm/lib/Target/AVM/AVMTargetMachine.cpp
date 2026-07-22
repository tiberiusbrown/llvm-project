//===-- AVMTargetMachine.cpp - AVM target machine ------------------------===//

#include "AVMTargetMachine.h"
#include "AVM.h"
#include "AVMMachineFunctionInfo.h"
#include "AVMTargetObjectFile.h"
#include "AVMTargetTransformInfo.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"

using namespace llvm;

static cl::opt<std::string>
    AVMTuneCPU("mtune", cl::desc("Tune AVM code for a specific processor"),
               cl::value_desc("cpu-name"), cl::init(""));

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeAVMTarget() {
  RegisterTargetMachine<AVMTargetMachine> X(getTheAVMTarget());
  PassRegistry &PR = *PassRegistry::getPassRegistry();
  initializeAVMAsmPrinterPass(PR);
  initializeAVMBranchPolarityPass(PR);
  initializeAVMDAGToDAGISelLegacyPass(PR);
  initializeAVMExpandPseudoPass(PR);
  initializeAVMFinalControlFlowPass(PR);
  initializeAVMProgramMemoryWideningPass(PR);
  initializeAVMServiceResultPass(PR);
}

AVMTargetMachine::AVMTargetMachine(const Target &T, const Triple &TT,
                                   StringRef CPU, StringRef FS,
                                   const TargetOptions &Options,
                                   std::optional<Reloc::Model>,
                                   std::optional<CodeModel::Model>,
                                   CodeGenOptLevel OL, bool)
    : CodeGenTargetMachineImpl(T, TT.computeDataLayout(), TT, CPU, FS, Options,
                               Reloc::Static, CodeModel::Large, OL),
      TLOF(std::make_unique<AVMTargetObjectFile>()) {
  initAsmInfo();
}

AVMTargetMachine::~AVMTargetMachine() = default;

TargetTransformInfo
AVMTargetMachine::getTargetTransformInfo(const Function &F) const {
  return TargetTransformInfo(std::make_unique<AVMTTIImpl>(this, F));
}

const AVMSubtarget *
AVMTargetMachine::getSubtargetImpl(const Function &F) const {
  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute TuneAttr = F.getFnAttribute("tune-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");

  std::string CPU =
      CPUAttr.isValid() ? CPUAttr.getValueAsString().str() : TargetCPU;
  if (CPU.empty())
    CPU = "avm1";
  std::string TuneCPU = TuneAttr.isValid() ? TuneAttr.getValueAsString().str()
                                           : std::string(AVMTuneCPU);
  if (TuneCPU.empty())
    TuneCPU = "avm-interpreter-32u4-v1";
  std::string FS =
      FSAttr.isValid() ? FSAttr.getValueAsString().str() : TargetFS;

  if (TuneCPU != "avm-interpreter-32u4-v1")
    report_fatal_error(Twine("unknown AVM tune CPU '") + TuneCPU + "'");

  auto &ST = SubtargetMap[CPU + "\n" + TuneCPU + "\n" + FS];
  if (!ST) {
    resetTargetOptions(F);
    ST = std::make_unique<AVMSubtarget>(TargetTriple, CPU, TuneCPU, FS, *this);
  }
  return ST.get();
}

namespace {
class AVMPassConfig final : public TargetPassConfig {
public:
  AVMPassConfig(AVMTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  void addIRPasses() override {
    TargetPassConfig::addIRPasses();

    if (getOptLevel() != CodeGenOptLevel::None) {
      addPass(createAVMProgramMemoryWideningPass());
      addPass(createInstructionCombiningPass());
    }

    addPass(createAtomicExpandLegacyPass());
  }

  bool addInstSelector() override {
    addPass(createAVMISelDag(getTM<AVMTargetMachine>(), getOptLevel()));
    return false;
  }

  void addPreRegAlloc() override { addPass(createAVMServiceResultPass()); }

  void addPreSched2() override {
    if (getOptLevel() != CodeGenOptLevel::None)
      addPass(&IfConverterID);
  }

  void addPreEmitPass() override {
    addPass(createAVMBranchPolarityPass());
    addPass(createAVMExpandPseudoPass());
    addPass(&DeadMachineInstructionElimID);
    addPass(createAVMFinalControlFlowPass());
  }
};
} // namespace

TargetPassConfig *AVMTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new AVMPassConfig(*this, PM);
}

MachineFunctionInfo *AVMTargetMachine::createMachineFunctionInfo(
    BumpPtrAllocator &Allocator, const Function &F,
    const TargetSubtargetInfo *STI) const {
  return AVMMachineFunctionInfo::create<AVMMachineFunctionInfo>(Allocator, F,
                                                                STI);
}
