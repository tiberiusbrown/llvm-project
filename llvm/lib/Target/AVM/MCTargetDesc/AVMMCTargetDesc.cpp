#include "AVMMCTargetDesc.h"
#include "AVMInstPrinter.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "AVMGenInstrInfo.inc"
#define GET_REGINFO_MC_DESC
#include "AVMGenRegisterInfo.inc"
#define GET_SUBTARGETINFO_MC_DESC
#include "AVMGenSubtargetInfo.inc"

namespace {

class AVMMCAsmInfo final : public MCAsmInfo {
public:
  AVMMCAsmInfo(const Triple &, const MCTargetOptions &) {
    CommentString = ";";
    SeparatorString = "\n";
    CodePointerSize = 3;
    CalleeSaveStackSlotSize = 2;
    MaxInstLength = 4;
    MinInstAlignment = 1;
    SupportsDebugInformation = false;
  }
};

static MCAsmInfo *createAVMMCAsmInfo(const MCRegisterInfo &, const Triple &TT,
                                     const MCTargetOptions &Options) {
  return new AVMMCAsmInfo(TT, Options);
}

static MCInstrInfo *createAVMMCInstrInfo() {
  auto *Info = new MCInstrInfo();
  InitAVMMCInstrInfo(Info);
  return Info;
}

static MCRegisterInfo *createAVMMCRegisterInfo(const Triple &) {
  auto *Info = new MCRegisterInfo();
  InitAVMMCRegisterInfo(Info, 0);
  return Info;
}

static MCSubtargetInfo *createAVMMCSubtargetInfo(const Triple &TT,
                                                 StringRef CPU,
                                                 StringRef FS) {
  StringRef EffectiveCPU = CPU.empty() ? "generic" : CPU;
  return createAVMMCSubtargetInfoImpl(TT, EffectiveCPU, EffectiveCPU, FS);
}

static MCInstPrinter *createAVMMCInstPrinter(const Triple &, unsigned Variant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI) {
  if (Variant != 0)
    return nullptr;
  return new AVMInstPrinter(MAI, MII, MRI);
}

} // namespace

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeAVMTargetMC() {
  Target &T = getTheAVMTarget();
  TargetRegistry::RegisterMCAsmInfo(T, createAVMMCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createAVMMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createAVMMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createAVMMCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createAVMMCInstPrinter);
  TargetRegistry::RegisterMCCodeEmitter(T, createAVMMCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(T, createAVMAsmBackend);
}
