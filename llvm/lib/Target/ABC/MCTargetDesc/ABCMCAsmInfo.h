//===-- ABCMCAsmInfo.h - ABC asm properties --------------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCMCASMINFO_H
#define LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class ABCMCAsmInfo : public MCAsmInfoELF {
public:
  explicit ABCMCAsmInfo(const Triple &TT, const MCTargetOptions &Options);

  virtual bool shouldOmitSectionDirective(StringRef SectionName) const override
  {
    return false;
  }
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCMCASMINFO_H
