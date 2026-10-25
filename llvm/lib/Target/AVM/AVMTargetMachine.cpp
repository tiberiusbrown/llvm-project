//===-- AVMTargetMachine.cpp - AVM target machine ------------------------===//

#include "AVMTargetMachine.h"
#include "AVM.h"
#include "AVMMachineFunctionInfo.h"
#include "AVMTargetObjectFile.h"
#include "AVMTargetTransformInfo.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/GlobalMerge.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Pass.h"
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
  initializeAVMTailDuplicationPass(PR);
  registerAVMTailDuplicationOptions();
  initializeAVMBranchPolarityPass(PR);
  initializeAVMCanonicalizeBooleansPass(PR);
  initializeAVMBytePHIsPass(PR);
  initializeAVMLoopStrengthReducePass(PR);
  initializeAVMConstantInitializersPass(PR);
  initializeAVMAggregateReturnsPass(PR);
  initializeAVMAggregateArgumentsPass(PR);
  initializeAVMDAGToDAGISelLegacyPass(PR);
  initializeAVMExpandPseudoPass(PR);
  initializeAVMFinalControlFlowPass(PR);
  initializeAVMLatePeepholePass(PR);
  initializeAVMAddressReusePass(PR);
  initializeAVMPrivateSpillCleanupPass(PR);
  initializeAVMProgramMemoryWideningPass(PR);
  initializeAVMSystemServiceRegionsPass(PR);
  initializeAVMExpandSystemServicesPass(PR);
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
  // Safety metadata is part of the AVM ABI, including LTO code generation.
  this->Options.EmitStackSizeSection = true;
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
// GlobalMerge's legacy FunctionPass runs in doInitialization(), before loop
// strength reduction regardless of its pipeline position. Run its module
// implementation here instead: merging must expose relative addresses to
// selection without changing which loop induction variables are selected.
class AVMGlobalMerge : public ModulePass {
  const AVMTargetMachine &TM;

public:
  static char ID;
  explicit AVMGlobalMerge(const AVMTargetMachine &TM) : ModulePass(ID), TM(TM) {}

  bool runOnModule(Module &M) override {
    GlobalMergeOptions Options;
    Options.MaxOffset = 224;
    Options.MergeExternal = false;
    ModuleAnalysisManager MAM;
    return !GlobalMergePass(&TM, Options).run(M, MAM).areAllPreserved();
  }

  StringRef getPassName() const override { return "AVM late global merging"; }
};
char AVMGlobalMerge::ID;

class AVMPassConfig final : public TargetPassConfig {
public:
  AVMPassConfig(AVMTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  void addIRPasses() override {
    TargetPassConfig::addIRPasses();

    if (getOptLevel() != CodeGenOptLevel::None) {
      addPass(createAVMAggregateReturnsPass());
      addPass(createAVMAggregateArgumentsPass());
      addPass(createAVMProgramMemoryWideningPass());
      addPass(createInstructionCombiningPass());
      addPass(createAVMConstantInitializersPass());
    }

    addPass(createAtomicExpandLegacyPass());
    addPass(createAVMStackTargetsPass());
  }

  bool addInstSelector() override {
    addPass(createAVMISelDag(getTM<AVMTargetMachine>(), getOptLevel()));
    return false;
  }

  bool addPreISel() override {
    // Give co-used local data a known relative layout before DAG address
    // selection. The selector can then cost a shared base against independent
    // pointers, including the slower displaced loads. Keep each merged object
    // within the positive displacement window and preserve external symbols.
    if (getOptLevel() != CodeGenOptLevel::None)
      addPass(new AVMGlobalMerge(getTM<AVMTargetMachine>()));
    return false;
  }

  void addCodeGenPrepare() override {
    TargetPassConfig::addCodeGenPrepare();
    // Preserve native byte representations after CodeGenPrepare has made its
    // placement decisions, but before DAG legalization loses their facts.
    if (getOptLevel() != CodeGenOptLevel::None)
      addPass(createAVMBytePHIsPass());
    if (getOptLevel() != CodeGenOptLevel::None)
      addPass(createAVMLoopStrengthReducePass());
  }

  void addPreRegAlloc() override {
    if (getOptLevel() != CodeGenOptLevel::None)
      addPass(createAVMCanonicalizeBooleansPass());
    addPass(createAVMSystemServiceRegionsPass());
  }

  void addPreSched2() override {
    if (getOptLevel() != CodeGenOptLevel::None)
      addPass(&IfConverterID);
  }

  bool addRegAssignAndRewriteOptimized() override {
    bool Rewritten = TargetPassConfig::addRegAssignAndRewriteOptimized();
    // Allocation introduces the spill stores and reloads. Optimize them while
    // private slots still have distinct identities, before coloring.
    if (Rewritten)
      addPass(createAVMPrivateSpillCleanupPass());
    return Rewritten;
  }

  void addPreEmitPass() override {
    if (getOptLevel() != CodeGenOptLevel::None)
      addPass(createAVMTailDuplicationPass(getOptLevel()));
    addPass(createAVMBranchPolarityPass());
    addPass(createAVMExpandSystemServicesPass());
    addPass(createAVMExpandPseudoPass());
    if (getOptLevel() != CodeGenOptLevel::None)
      addPass(createAVMAddressReusePass());
    if (getOptLevel() != CodeGenOptLevel::None)
      addPass(createAVMLatePeepholePass());
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
