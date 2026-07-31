//===-- AVMSystemServiceInfo.cpp - AVM system-service metadata -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVMSystemServiceInfo.h"
#include "AVM.h"
#include "AVMInstrInfo.h"
#include "AVMRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"

#include <cassert>

using namespace llvm;

namespace {

// Generate the numeric and assembly-name constants from the MC-safe mapping.
// Descriptors therefore reference, rather than duplicate, the authoritative
// identities in AVMSystemCalls.inc.
#define AVM_SYS_PSEUDO(Pseudo, ID, AsmName)                                    \
  static constexpr uint8_t Pseudo##ServiceID = ID;                             \
  static constexpr StringLiteral Pseudo##ServiceName = #AsmName;
#define AVM_NO_PSEUDO(Pseudo, ID, AsmName)
#define AVM_SYS_DEF(ID, AsmName, PseudoKind, Pseudo, IntrinsicKind, Intrinsic, \
                    CostKind, Cost)                                            \
  PseudoKind(Pseudo, ID, AsmName)
#include "AVMSystemCalls.inc"
#undef AVM_SYS_PSEUDO
#undef AVM_NO_PSEUDO

using VK = AVMServiceValueKind;
using PP = AVMServicePointerPolicy;
using MB = AVMServiceMemoryBaseKind;
using MS = AVMServiceMemorySizeKind;

static constexpr AVMServiceInputInfo DebugPutcInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
};

static constexpr AVMServiceOutputInfo MillisOutputs[] = {
    {0, VK::I16, AVM::R4, -1},
};
static constexpr AVMServiceOutputInfo Millis32Outputs[] = {
    {0, VK::I32, AVM::R4R5, -1},
};

static constexpr AVMServiceInputInfo DisplayInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
};
static constexpr AVMServiceMemoryAccessInfo DisplayMemory[] = {
    {MB::FixedGlobal, 0, "__avm_framebuffer", 0, MachineMemOperand::MOLoad,
     MS::Constant, 0, 1024},
    {MB::FixedGlobal, 0, "__avm_framebuffer", 0, MachineMemOperand::MOStore,
     MS::Constant, 0, 1024},
};

static constexpr AVMServiceInputInfo UnaryMathInputs[] = {
    {0, VK::F32, AVM::R4R5, PP::None, true},
};
static constexpr AVMServiceOutputInfo UnaryMathOutputs[] = {
    {0, VK::F32, AVM::R4R5, 0},
};
static constexpr AVMServiceInputInfo BinaryMathInputs[] = {
    {0, VK::F32, AVM::R4R5, PP::None, true},
    {1, VK::F32, AVM::R6R7, PP::None, true},
};
static constexpr AVMServiceOutputInfo BinaryMathOutputs[] = {
    {0, VK::F32, AVM::R4R5, 0},
};

static constexpr AVMServiceInputInfo MemoryInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
    {2, VK::I16, AVM::R6, PP::None, true},
};
static constexpr AVMServiceOutputInfo TiedR4Output[] = {
    {0, VK::I16, AVM::R4, 0},
};
static constexpr AVMServiceMemoryAccessInfo CopyMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOStore,
     MS::LogicalArgument, 2, 0},
    {MB::LogicalArgument, 1, "", 0, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 2, 0},
};
static constexpr AVMServiceMemoryAccessInfo SetMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOStore,
     MS::LogicalArgument, 2, 0},
};

static constexpr AVMServiceInputInfo MemcpyPInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::ProgramPointer, AVM::R6R7, PP::IgnorePadding, true},
    {2, VK::I16, AVM::R5, PP::None, true},
};
static constexpr AVMServiceMemoryAccessInfo MemcpyPMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOStore,
     MS::LogicalArgument, 2, 0},
    {MB::LogicalArgument, 1, "", 1, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 2, 0},
};

static constexpr AVMServiceInputInfo MemcmpPInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::ProgramPointer, AVM::R6R7, PP::IgnorePadding, true},
    {2, VK::I16, AVM::R5, PP::None, true},
};
static constexpr AVMServiceMemoryAccessInfo MemcmpPMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 2, 0},
    {MB::LogicalArgument, 1, "", 1, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 2, 0},
};

static constexpr AVMServiceInputInfo StrcmpPInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::ProgramPointer, AVM::R6R7, PP::IgnorePadding, true},
};
static constexpr AVMServiceMemoryAccessInfo StrcmpPMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 1, "", 1, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
};

static constexpr AVMServiceInputInfo StrlenPInputs[] = {
    {0, VK::ProgramPointer, AVM::R6R7, PP::IgnorePadding, true},
};
static constexpr AVMServiceOutputInfo R4Output[] = {
    {0, VK::I16, AVM::R4, -1},
};
static constexpr AVMServiceMemoryAccessInfo StrlenPMemory[] = {
    {MB::LogicalArgument, 0, "", 1, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
};

static constexpr AVMServiceInputInfo StrncpyPInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::ProgramPointer, AVM::R6R7, PP::IgnorePadding, true},
    {2, VK::I16, AVM::R5, PP::None, true},
};
static constexpr AVMServiceMemoryAccessInfo StrncpyPMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOStore,
     MS::LogicalArgument, 2, 0},
    {MB::LogicalArgument, 1, "", 1, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 2, 0},
};
static constexpr AVMServiceMemoryAccessInfo StrncatPMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOStore,
     MS::AfterPointer, 0, 0},
    {MB::LogicalArgument, 1, "", 1, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 2, 0},
};

static constexpr AVMServiceInputInfo CompareInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
    {2, VK::I16, AVM::R6, PP::None, true},
};
static constexpr AVMServiceMemoryAccessInfo CompareMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 2, 0},
    {MB::LogicalArgument, 1, "", 0, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 2, 0},
};
static constexpr AVMServiceInputInfo StrcmpInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
};
static constexpr AVMServiceMemoryAccessInfo StrcmpMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 1, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
};
static constexpr AVMServiceInputInfo StrlenInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
};
static constexpr AVMServiceMemoryAccessInfo StrlenMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
};
static constexpr AVMServiceMemoryAccessInfo StrncpyMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOStore,
     MS::LogicalArgument, 2, 0},
    {MB::LogicalArgument, 1, "", 0, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 2, 0},
};
static constexpr AVMServiceMemoryAccessInfo StrncatMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOStore,
     MS::AfterPointer, 0, 0},
    {MB::LogicalArgument, 1, "", 0, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 2, 0},
};

static constexpr AVMServiceInputInfo VsnprintfInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true}, // destination/result
    {1, VK::I16, AVM::R5, PP::None, true}, // destination size
    {2, VK::I16, AVM::R6, PP::None, true}, // RAM format
    {3, VK::I16, AVM::R2, PP::None, true}, // va_list cursor
};
static constexpr AVMServiceInputInfo VsnprintfPInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
    {2, VK::ProgramPointer, AVM::R6R7, PP::IgnorePadding, true},
    {3, VK::I16, AVM::R2, PP::None, true},
};
// Besides their explicit format and va_list pointers, both services can read
// arbitrary AS0 strings through %s and arbitrary AS1 strings through %S.
// Unknown-address-space MMOs preserve those indirect effects without claiming
// that the pointees are based on the va_list object itself.
static constexpr AVMServiceMemoryAccessInfo VsnprintfMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOStore,
     MS::LogicalArgument, 1, 0},
    {MB::LogicalArgument, 2, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 3, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::UnknownAddressSpace, 0, "", 0, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
    {MB::UnknownAddressSpace, 0, "", 1, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
};
static constexpr AVMServiceMemoryAccessInfo VsnprintfPMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOStore,
     MS::LogicalArgument, 1, 0},
    {MB::LogicalArgument, 2, "", 1, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 3, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::UnknownAddressSpace, 0, "", 0, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
    {MB::UnknownAddressSpace, 0, "", 1, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
};

static constexpr AVMServiceInputInfo DebugPrintfVPInputs[] = {
    {0, VK::ProgramPointer, AVM::R6R7, PP::IgnorePadding, true},
    {1, VK::I16, AVM::R2, PP::None, true},
};
static constexpr AVMServiceMemoryAccessInfo DebugPrintfVPMemory[] = {
    {MB::LogicalArgument, 0, "", 1, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 1, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::UnknownAddressSpace, 0, "", 0, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
    {MB::UnknownAddressSpace, 0, "", 1, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
};

static constexpr AVMServiceOutputInfo TextCursorOutput[] = {
    {0, VK::I32, AVM::R4R5, -1},
};

static constexpr AVMServiceInputInfo SetTextFontInputs[] = {
    {0, VK::ProgramPointer, AVM::R4R5, PP::IgnorePadding, true},
    {1, VK::I16, MCPhysReg(), PP::None, false}, // selected-text-state anchor
};
static constexpr AVMServiceMemoryAccessInfo SetTextFontMemory[] = {
    {MB::LogicalArgument, 0, "", 1, MachineMemOperand::MOLoad, MS::Constant, 0,
     3},
    {MB::LogicalArgument, 1, "__avm_text_state", 0,
     MachineMemOperand::MOStore, MS::Constant, 0, 7},
};

static constexpr AVMServiceInputInfo SetTextModeInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, MCPhysReg(), PP::None, false}, // selected-text-state anchor
};
static constexpr AVMServiceMemoryAccessInfo SetTextModeMemory[] = {
    // The mode is one byte within the seven-byte state object. Model the whole
    // object so mode changes alias every draw's selected-state read.
    {MB::LogicalArgument, 1, "__avm_text_state", 0,
     MachineMemOperand::MOStore, MS::Constant, 0, 7},
};

static constexpr AVMServiceInputInfo DrawTextInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
    {2, VK::I16, AVM::R6, PP::None, true},
    {3, VK::I16, MCPhysReg(), PP::None, false}, // text-state anchor
    {4, VK::I16, MCPhysReg(), PP::None, false}, // framebuffer anchor
};
static constexpr AVMServiceInputInfo DrawTextPInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
    {2, VK::ProgramPointer, AVM::R6R7, PP::IgnorePadding, true},
    {3, VK::I16, MCPhysReg(), PP::None, false},
    {4, VK::I16, MCPhysReg(), PP::None, false},
};
static constexpr AVMServiceInputInfo DrawTextFVInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
    {2, VK::I16, AVM::R6, PP::None, true},
    {3, VK::I16, AVM::R2, PP::None, true},
    {4, VK::I16, MCPhysReg(), PP::None, false},
    {5, VK::I16, MCPhysReg(), PP::None, false},
};
static constexpr AVMServiceInputInfo DrawTextFVPInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
    {2, VK::ProgramPointer, AVM::R6R7, PP::IgnorePadding, true},
    {3, VK::I16, AVM::R2, PP::None, true},
    {4, VK::I16, MCPhysReg(), PP::None, false},
    {5, VK::I16, MCPhysReg(), PP::None, false},
};

static constexpr AVMServiceMemoryAccessInfo DrawTextMemory[] = {
    {MB::LogicalArgument, 2, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 3, "__avm_text_state", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 7},
    {MB::LogicalArgument, 4, "__avm_framebuffer", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 1024},
    {MB::LogicalArgument, 4, "__avm_framebuffer", 0,
     MachineMemOperand::MOStore, MS::Constant, 0, 1024},
    // Glyph records and images are reached through the cached glyph-table
    // pointer in selected text state.
    {MB::UnknownAddressSpace, 0, "", 1, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
};
static constexpr AVMServiceMemoryAccessInfo DrawTextPMemory[] = {
    {MB::LogicalArgument, 2, "", 1, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 3, "__avm_text_state", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 7},
    {MB::LogicalArgument, 4, "__avm_framebuffer", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 1024},
    {MB::LogicalArgument, 4, "__avm_framebuffer", 0,
     MachineMemOperand::MOStore, MS::Constant, 0, 1024},
    {MB::UnknownAddressSpace, 0, "", 1, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
};

static constexpr AVMServiceMemoryAccessInfo DrawTextFVMemory[] = {
    {MB::LogicalArgument, 2, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 3, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 4, "__avm_text_state", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 7},
    {MB::LogicalArgument, 5, "__avm_framebuffer", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 1024},
    {MB::LogicalArgument, 5, "__avm_framebuffer", 0,
     MachineMemOperand::MOStore, MS::Constant, 0, 1024},
    {MB::UnknownAddressSpace, 0, "", 0, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
    {MB::UnknownAddressSpace, 0, "", 1, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
};
static constexpr AVMServiceMemoryAccessInfo DrawTextFVPMemory[] = {
    {MB::LogicalArgument, 2, "", 1, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 3, "", 0, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
    {MB::LogicalArgument, 4, "__avm_text_state", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 7},
    {MB::LogicalArgument, 5, "__avm_framebuffer", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 1024},
    {MB::LogicalArgument, 5, "__avm_framebuffer", 0,
     MachineMemOperand::MOStore, MS::Constant, 0, 1024},
    {MB::UnknownAddressSpace, 0, "", 0, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
    {MB::UnknownAddressSpace, 0, "", 1, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
};

static constexpr AVMServiceInputInfo SpriteInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
    {2, VK::ProgramPointer, AVM::R2R3, PP::IgnorePadding, true},
    {3, VK::I16, AVM::R6, PP::None, true},
    {4, VK::I16, MCPhysReg(), PP::None, false},
};
static constexpr AVMServiceMemoryAccessInfo SpriteMemory[] = {
    {MB::LogicalArgument, 4, "__avm_framebuffer", 0, MachineMemOperand::MOLoad,
     MS::Constant, 0, 1024},
    {MB::LogicalArgument, 4, "__avm_framebuffer", 0, MachineMemOperand::MOStore,
     MS::Constant, 0, 1024},
    {MB::LogicalArgument, 2, "", 1, MachineMemOperand::MOLoad, MS::AfterPointer,
     0, 0},
};

static constexpr AVMServiceInputInfo SetSpriteInputs[] = {
    {0, VK::ProgramPointer, AVM::R4R5, PP::IgnorePadding, true},

    // Compiler-only memory anchor for:
    //   uint24_t pointer;
    //   uint8_t width;
    //   uint8_t height;
    {1, VK::I16, MCPhysReg(), PP::None, false},
};
static constexpr AVMServiceMemoryAccessInfo SetSpriteMemory[] = {
    // width and height at offsets 0 and 1
    {MB::LogicalArgument, 0, "", 1,
     MachineMemOperand::MOLoad, MS::Constant, 0, 2},

    // raw pointer + width + height
    {MB::LogicalArgument, 1, "__avm_current_sprite", 0,
     MachineMemOperand::MOStore, MS::Constant, 0, 5},
};

static constexpr AVMServiceInputInfo SpriteNoPtrInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true}, // x
    {1, VK::I16, AVM::R5, PP::None, true}, // y
    {2, VK::I16, AVM::R6, PP::None, true}, // frame

    // Compiler-only selected-sprite-state anchor.
    {3, VK::I16, MCPhysReg(), PP::None, false},

    // Compiler-only framebuffer anchor.
    {4, VK::I16, MCPhysReg(), PP::None, false},
};
static constexpr AVMServiceMemoryAccessInfo SpriteNoPtrMemory[] = {
    {MB::LogicalArgument, 3, "__avm_current_sprite", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 5},

    {MB::LogicalArgument, 4, "__avm_framebuffer", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 1024},

    {MB::LogicalArgument, 4, "__avm_framebuffer", 0,
     MachineMemOperand::MOStore, MS::Constant, 0, 1024},

    // Selected sprite data at a runtime-cached, unknown AS1 address.
    {MB::UnknownAddressSpace, 0, "", 1,
     MachineMemOperand::MOLoad, MS::AfterPointer, 0, 0},
};

static constexpr AVMServiceInputInfo FilledRectInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true}, // x
    {1, VK::I16, AVM::R5, PP::None, true}, // y
    {2, VK::I16, AVM::R6, PP::None, true}, // zero-extended width
    {3, VK::I16, AVM::R7, PP::None, true}, // zero-extended height

    // Compiler-only framebuffer anchor.
    {4, VK::I16, MCPhysReg(), PP::None, false},
};
static constexpr AVMServiceMemoryAccessInfo FilledRectMemory[] = {
    {MB::LogicalArgument, 4, "__avm_framebuffer", 0,
     MachineMemOperand::MOLoad, MS::Constant, 0, 1024},

    {MB::LogicalArgument, 4, "__avm_framebuffer", 0,
     MachineMemOperand::MOStore, MS::Constant, 0, 1024},
};

// The persisted prefix begins at a fixed architectural address, but its size
// is read from the image header at runtime and is not an intrinsic operand.
// Until the backend has a first-class saved-prefix object, use an unknown AS0
// base and unknown size.  This is conservative with respect to every possible
// .saved access while still distinguishing save (read) from load (write).
static constexpr AVMServiceMemoryAccessInfo PersistenceSaveMemory[] = {
    {MB::UnknownAddressSpace, 0, "", 0, MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
};
static constexpr AVMServiceMemoryAccessInfo PersistenceLoadMemory[] = {
    {MB::UnknownAddressSpace, 0, "", 0, MachineMemOperand::MOStore,
     MS::AfterPointer, 0, 0},
};

#define SERVICE(Pseudo, Inputs, Outputs, Memory)                               \
  static constexpr AVMSystemServiceInfo Pseudo##Info = {                       \
      AVM::Pseudo, Pseudo##ServiceID, Pseudo##ServiceName,                     \
      Inputs,      Outputs,           Memory}

SERVICE(SYS_DEBUG_PUTC_PSEUDO, DebugPutcInputs, {}, {});
SERVICE(SYS_DEBUG_BREAK_PSEUDO, {}, {}, {});
SERVICE(SYS_MILLIS_PSEUDO, {}, MillisOutputs, {});
SERVICE(SYS_MILLIS32_PSEUDO, {}, Millis32Outputs, {});
SERVICE(SYS_BUTTONS_PSEUDO, {}, R4Output, {});
SERVICE(SYS_IDLE_PSEUDO, {}, {}, {});
SERVICE(SYS_GENERATE_RANDOM_SEED_PSEUDO, {}, R4Output, {});
SERVICE(SYS_SAVE_PSEUDO, {}, {}, PersistenceSaveMemory);
SERVICE(SYS_LOAD_PSEUDO, {}, R4Output, PersistenceLoadMemory);
SERVICE(SYS_SAVE_EXISTS_PSEUDO, {}, R4Output, {});
SERVICE(SYS_DISPLAY_PSEUDO, DisplayInputs, {}, DisplayMemory);
SERVICE(SYS_SINF_PSEUDO, UnaryMathInputs, UnaryMathOutputs, {});
SERVICE(SYS_COSF_PSEUDO, UnaryMathInputs, UnaryMathOutputs, {});
SERVICE(SYS_ATAN2F_PSEUDO, BinaryMathInputs, BinaryMathOutputs, {});
SERVICE(SYS_TANF_PSEUDO, UnaryMathInputs, UnaryMathOutputs, {});
SERVICE(SYS_EXPF_PSEUDO, UnaryMathInputs, UnaryMathOutputs, {});
SERVICE(SYS_LOGF_PSEUDO, UnaryMathInputs, UnaryMathOutputs, {});
SERVICE(SYS_LOG2F_PSEUDO, UnaryMathInputs, UnaryMathOutputs, {});
SERVICE(SYS_LOG10F_PSEUDO, UnaryMathInputs, UnaryMathOutputs, {});
SERVICE(SYS_POWF_PSEUDO, BinaryMathInputs, BinaryMathOutputs, {});
SERVICE(SYS_HYPOTF_PSEUDO, BinaryMathInputs, BinaryMathOutputs, {});
SERVICE(SYS_FMODF_PSEUDO, BinaryMathInputs, BinaryMathOutputs, {});
SERVICE(SYS_MEMCPY_PSEUDO, MemoryInputs, TiedR4Output, CopyMemory);
SERVICE(SYS_MEMCPY_P_PSEUDO, MemcpyPInputs, TiedR4Output, MemcpyPMemory);
SERVICE(SYS_MEMSET_PSEUDO, MemoryInputs, TiedR4Output, SetMemory);
SERVICE(SYS_MEMMOVE_PSEUDO, MemoryInputs, TiedR4Output, CopyMemory);
SERVICE(SYS_MEMCMP_P_PSEUDO, MemcmpPInputs, TiedR4Output, MemcmpPMemory);
SERVICE(SYS_STRCMP_P_PSEUDO, StrcmpPInputs, TiedR4Output, StrcmpPMemory);
SERVICE(SYS_STRLEN_P_PSEUDO, StrlenPInputs, R4Output, StrlenPMemory);
SERVICE(SYS_STRNCPY_P_PSEUDO, StrncpyPInputs, TiedR4Output, StrncpyPMemory);
SERVICE(SYS_STRNCAT_P_PSEUDO, StrncpyPInputs, TiedR4Output, StrncatPMemory);
SERVICE(SYS_MEMCMP_PSEUDO, CompareInputs, TiedR4Output, CompareMemory);
SERVICE(SYS_STRCMP_PSEUDO, StrcmpInputs, TiedR4Output, StrcmpMemory);
SERVICE(SYS_STRLEN_PSEUDO, StrlenInputs, TiedR4Output, StrlenMemory);
SERVICE(SYS_STRNCPY_PSEUDO, MemoryInputs, TiedR4Output, StrncpyMemory);
SERVICE(SYS_STRNCAT_PSEUDO, MemoryInputs, TiedR4Output, StrncatMemory);
SERVICE(SYS_VSNPRINTF_PSEUDO, VsnprintfInputs, TiedR4Output,
        VsnprintfMemory);
SERVICE(SYS_VSNPRINTF_P_PSEUDO, VsnprintfPInputs, TiedR4Output,
        VsnprintfPMemory);
SERVICE(SYS_DEBUG_PRINTFV_P_PSEUDO, DebugPrintfVPInputs, R4Output,
        DebugPrintfVPMemory);
SERVICE(SYS_SET_TEXT_FONT_PSEUDO, SetTextFontInputs, {}, SetTextFontMemory);
SERVICE(SYS_SET_TEXT_MODE_PSEUDO, SetTextModeInputs, {}, SetTextModeMemory);
SERVICE(SYS_DRAW_TEXT_PSEUDO, DrawTextInputs, TextCursorOutput,
        DrawTextMemory);
SERVICE(SYS_DRAW_TEXT_P_PSEUDO, DrawTextPInputs, TextCursorOutput,
        DrawTextPMemory);
SERVICE(SYS_DRAW_TEXTFV_PSEUDO, DrawTextFVInputs, TextCursorOutput,
        DrawTextFVMemory);
SERVICE(SYS_DRAW_TEXTFV_P_PSEUDO, DrawTextFVPInputs, TextCursorOutput,
        DrawTextFVPMemory);
SERVICE(SYS_DRAW_SPRITE_OVERWRITE_PSEUDO, SpriteInputs, {}, SpriteMemory);
SERVICE(SYS_DRAW_SPRITE_PLUS_MASK_PSEUDO, SpriteInputs, {}, SpriteMemory);
SERVICE(SYS_DRAW_SPRITE_SELF_MASKED_PSEUDO, SpriteInputs, {}, SpriteMemory);
SERVICE(SYS_DRAW_SPRITE_ERASE_PSEUDO, SpriteInputs, {}, SpriteMemory);
SERVICE(SYS_SET_SPRITE_PSEUDO, SetSpriteInputs, {}, SetSpriteMemory);
SERVICE(SYS_DRAW_OVERWRITE_PSEUDO, SpriteNoPtrInputs, {}, SpriteNoPtrMemory);
SERVICE(SYS_DRAW_PLUS_MASK_PSEUDO, SpriteNoPtrInputs, {}, SpriteNoPtrMemory);
SERVICE(SYS_DRAW_SELF_MASKED_PSEUDO, SpriteNoPtrInputs, {}, SpriteNoPtrMemory);
SERVICE(SYS_DRAW_ERASE_PSEUDO, SpriteNoPtrInputs, {}, SpriteNoPtrMemory);
SERVICE(SYS_DRAW_FILLED_RECT_WHITE_PSEUDO, FilledRectInputs, {},
        FilledRectMemory);
SERVICE(SYS_DRAW_FILLED_RECT_BLACK_PSEUDO, FilledRectInputs, {},
        FilledRectMemory);

#undef SERVICE

#ifndef NDEBUG
static int getGeneratedServiceID(unsigned Opcode) {
  switch (Opcode) {
#define AVM_SYS_PSEUDO(Pseudo, ID)                                             \
  case AVM::Pseudo:                                                            \
    return ID;
#define AVM_NO_PSEUDO(Pseudo, ID)
#define AVM_SYS_DEF(ID, AsmName, PseudoKind, Pseudo, IntrinsicKind, Intrinsic, \
                    CostKind, Cost)                                            \
  PseudoKind(Pseudo, ID)
#include "AVMSystemCalls.inc"
#undef AVM_SYS_PSEUDO
#undef AVM_NO_PSEUDO
  default:
    return -1;
  }
}
#endif

} // namespace

const AVMSystemServiceInfo *llvm::getAVMSystemServiceInfo(unsigned Opcode) {
  const AVMSystemServiceInfo *Info = nullptr;
  switch (Opcode) {
#define AVM_SYS_PSEUDO(Pseudo, ID)                                             \
  case AVM::Pseudo:                                                            \
    Info = &Pseudo##Info;                                                      \
    break;
#define AVM_NO_PSEUDO(Pseudo, ID)
#define AVM_SYS_DEF(ID, AsmName, PseudoKind, Pseudo, IntrinsicKind, Intrinsic, \
                    CostKind, Cost)                                            \
  PseudoKind(Pseudo, ID)
#include "AVMSystemCalls.inc"
#undef AVM_SYS_PSEUDO
#undef AVM_NO_PSEUDO
  default:
    return nullptr;
  }
#ifndef NDEBUG
  assert(Info && getGeneratedServiceID(Opcode) == Info->ServiceID &&
         "AVM service descriptor disagrees with AVMSystemCalls.inc");
#endif
  return Info;
}

const AVMSystemServiceInfo &
llvm::getRequiredAVMSystemServiceInfo(unsigned Opcode) {
  const AVMSystemServiceInfo *Info = getAVMSystemServiceInfo(Opcode);
  if (!Info)
    llvm_unreachable("expected descriptor-backed AVM system service");
  return *Info;
}

bool llvm::isAVMSystemService(unsigned Opcode) {
  return getAVMSystemServiceInfo(Opcode) != nullptr;
}

int llvm::getAVMSystemServiceID(unsigned Opcode) {
  if (const auto *Info = getAVMSystemServiceInfo(Opcode))
    return Info->ServiceID;
  return -1;
}

const TargetRegisterClass *
llvm::getAVMFixedRegisterClass(MCPhysReg Reg, AVMServiceValueKind Kind) {
  const bool IsWide = Kind == AVMServiceValueKind::I32 ||
                      Kind == AVMServiceValueKind::F32 ||
                      Kind == AVMServiceValueKind::ProgramPointer;
  if (IsWide) {
    switch (Reg) {
    case AVM::R0R1:
      return &AVM::Q0OnlyRegClass;
    case AVM::R2R3:
      return &AVM::Q1OnlyRegClass;
    case AVM::R4R5:
      return &AVM::Q2OnlyRegClass;
    case AVM::R6R7:
      return &AVM::Q3OnlyRegClass;
    default:
      return nullptr;
    }
  }

  switch (Reg) {
  case AVM::R0:
    return &AVM::R0OnlyRegClass;
  case AVM::R1:
    return &AVM::R1OnlyRegClass;
  case AVM::R2:
    return &AVM::R2OnlyRegClass;
  case AVM::R3:
    return &AVM::R3OnlyRegClass;
  case AVM::R4:
    return &AVM::R4OnlyRegClass;
  case AVM::R5:
    return &AVM::R5OnlyRegClass;
  case AVM::R6:
    return &AVM::R6OnlyRegClass;
  case AVM::R7:
    return &AVM::R7OnlyRegClass;
  default:
    return nullptr;
  }
}

const TargetRegisterClass *
llvm::getAVMGeneralRegisterClass(AVMServiceValueKind Kind) {
  switch (Kind) {
  case AVMServiceValueKind::I16:
    return &AVM::GPR16RegClass;
  case AVMServiceValueKind::I32:
  case AVMServiceValueKind::F32:
    return &AVM::GPR32RegClass;
  case AVMServiceValueKind::ProgramPointer:
    return &AVM::ProgPtrGPR32RegClass;
  }
  llvm_unreachable("unknown AVM service value kind");
}

bool llvm::isAVMFixedServiceRegisterClass(const TargetRegisterClass *RC) {
  for (MCPhysReg Reg :
       {AVM::R0, AVM::R1, AVM::R2, AVM::R3, AVM::R4, AVM::R5, AVM::R6, AVM::R7})
    if (RC == getAVMFixedRegisterClass(Reg, AVMServiceValueKind::I16))
      return true;
  for (MCPhysReg Reg : {AVM::R0R1, AVM::R2R3, AVM::R4R5, AVM::R6R7})
    if (RC == getAVMFixedRegisterClass(Reg, AVMServiceValueKind::I32))
      return true;
  return false;
}

int llvm::getAVMServiceInputOperandIndex(const AVMSystemServiceInfo &Info,
                                         unsigned LogicalArgumentIndex) {
  unsigned OperandIndex = Info.Outputs.size();
  for (const AVMServiceInputInfo &Input : Info.Inputs) {
    if (!Input.PassToMachine)
      continue;
    if (Input.LogicalArgumentIndex == LogicalArgumentIndex)
      return OperandIndex;
    ++OperandIndex;
  }
  return -1;
}
