//===-- ABCMCTargetDesc.cpp - ABC target descriptions ---------------------===//

#include "ABCMCTargetDesc.h"
#include "ABCMCAsmInfo.h"
#include "ABCInstPrinter.h"
#include "TargetInfo/ABCTargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#define GET_INSTRINFO_MC_HELPERS
#include "ABCGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "ABCGenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "ABCGenSubtargetInfo.inc"

static MCInstrInfo *createABCMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitABCMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createABCMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitABCMCRegisterInfo(X, ABC::STK);
  return X;
}

static MCSubtargetInfo *createABCMCSubtargetInfo(const Triple &TT,
                                                StringRef CPU, StringRef FS) {
  return createABCMCSubtargetInfoImpl(TT, CPU.empty() ? "generic" : CPU,
                                      CPU.empty() ? "generic" : CPU, FS);
}

static MCAsmInfo *createABCMCAsmInfo(const MCRegisterInfo &MRI,
                                    const Triple &TT,
                                    const MCTargetOptions &Options) {
  return new ABCMCAsmInfo(TT, Options);
}

static MCInstPrinter *createABCMCInstPrinter(const Triple &T,
                                             unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new ABCInstPrinter(MAI, MII, MRI);
  return nullptr;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeABCTargetMC() {
  Target &T = getTheABCTarget();
  TargetRegistry::RegisterMCAsmInfo(T, createABCMCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createABCMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createABCMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createABCMCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createABCMCInstPrinter);
  TargetRegistry::RegisterMCCodeEmitter(T, createABCMCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(T, createABCAsmBackend);
}
