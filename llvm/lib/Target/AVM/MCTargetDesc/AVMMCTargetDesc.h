#ifndef LLVM_LIB_TARGET_AVM_MCTARGETDESC_AVMMCTARGETDESC_H
#define LLVM_LIB_TARGET_AVM_MCTARGETDESC_AVMMCTARGETDESC_H

#include <cstdint>
#include <memory>

namespace llvm {
class MCAsmBackend;
class MCContext;
class MCCodeEmitter;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCTargetOptions;
class Target;

MCAsmBackend *createAVMAsmBackend(const Target &, const MCSubtargetInfo &,
                                  const MCRegisterInfo &,
                                  const MCTargetOptions &);
MCCodeEmitter *createAVMMCCodeEmitter(const MCInstrInfo &, MCContext &);
std::unique_ptr<MCObjectTargetWriter>
createAVMELFObjectWriter(uint8_t OSABI);
} // namespace llvm

#define GET_REGINFO_ENUM
#include "AVMGenRegisterInfo.inc"
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "AVMGenInstrInfo.inc"
#define GET_SUBTARGETINFO_ENUM
#include "AVMGenSubtargetInfo.inc"

#endif
