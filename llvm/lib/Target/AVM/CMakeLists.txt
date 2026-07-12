add_llvm_component_group(AVM)

set(LLVM_TARGET_DEFINITIONS AVM.td)
tablegen(LLVM AVMGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM AVMGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM AVMGenSubtargetInfo.inc -gen-subtarget)
add_public_tablegen_target(AVMCommonTableGen)

add_llvm_target(AVMCodeGen
  AVMInstrInfo.cpp
  AVMRegisterInfo.cpp
  AVMSubtarget.cpp
  AVMTargetMachine.cpp

  DEPENDS
  AVMCommonTableGen

  LINK_COMPONENTS
  AVMDesc
  AVMInfo
  CodeGen
  CodeGenTypes
  Core
  MC
  Support
  Target
  TargetParser

  ADD_TO_COMPONENT
  AVM
)

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
