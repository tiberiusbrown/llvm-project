add_llvm_component_group(AVM)

# The MC sublibraries include generated target descriptions from this target
# directory. `add_llvm_target` used to supply these inherited include paths;
# retain them now that AVM is intentionally not a code-generation target.
include_directories(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR})

set(LLVM_TARGET_DEFINITIONS AVM.td)
tablegen(LLVM AVMGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM AVMGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM AVMGenSubtargetInfo.inc -gen-subtarget)
add_public_tablegen_target(AVMCommonTableGen)

# This is deliberately an MC-only target aggregate. LLVM's component mapping
# needs a target library for tools that link `${LLVM_TARGETS_TO_BUILD}`, while
# AVM must not register or pull in any code-generation implementation.
add_llvm_component_library(LLVMAVM
  AVMMCOnly.cpp

  DEPENDS
  AVMCommonTableGen

  LINK_COMPONENTS
  AVMDesc
  AVMInfo
  MC
  Support

  ADD_TO_COMPONENT
  AVM
)

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
