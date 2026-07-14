//===-- ABCMCAsmInfo.cpp - ABC asm properties -----------------------------===//

#include "ABCMCAsmInfo.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

ABCMCAsmInfo::ABCMCAsmInfo(const Triple &TT, const MCTargetOptions &Options)
    : MCAsmInfoELF(Options) {
  CodePointerSize = 3;
  CalleeSaveStackSlotSize = 3;
  MinInstAlignment = 1;
  CommentString = ";";
  InternalSymbolPrefix = ".L";
  SupportsDebugInformation = true;
}
