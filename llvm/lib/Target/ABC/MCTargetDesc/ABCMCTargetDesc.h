//===-- ABCMCTargetDesc.h - ABC target descriptions ------------*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCMCTARGETDESC_H
#define LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCMCTARGETDESC_H

#include "llvm/Config/config.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/DataTypes.h"
#include <memory>

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCTargetOptions;
class Target;
class Triple;
class raw_pwrite_stream;

MCCodeEmitter *createABCMCCodeEmitter(const MCInstrInfo &MCII,
                                      MCContext &Ctx);
MCAsmBackend *createABCAsmBackend(const Target &T, const MCSubtargetInfo &STI,
                                  const MCRegisterInfo &MRI,
                                  const MCTargetOptions &Options);
std::unique_ptr<MCObjectTargetWriter> createABCELFObjectWriter(uint8_t OSABI);
} // namespace llvm

#define GET_REGINFO_ENUM
#include "ABCGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "ABCGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "ABCGenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_ABC_MCTARGETDESC_ABCMCTARGETDESC_H
