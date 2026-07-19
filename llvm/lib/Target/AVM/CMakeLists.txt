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
tablegen(LLVM AVMGenCallingConv.inc -gen-callingconv)
tablegen(LLVM AVMGenDAGISel.inc -gen-dag-isel)
add_public_tablegen_target(AVMCommonTableGen)

add_llvm_target(AVMCodeGen
  AVMAsmPrinter.cpp
  AVMCostModel.cpp
  AVMExpandPseudoInsts.cpp
  AVMFrameLowering.cpp
  AVMISelDAGToDAG.cpp
  AVMISelLowering.cpp
  AVMInstrInfo.cpp
  AVMMachineFunctionInfo.cpp
  AVMMCInstLower.cpp
  AVMRegisterInfo.cpp
  AVMSelectionDAGInfo.cpp
  AVMSubtarget.cpp
  AVMTargetMachine.cpp

  DEPENDS
  AVMCommonTableGen

  LINK_COMPONENTS
  AsmPrinter
  AVMDesc
  AVMInfo
  CodeGen
  CodeGenTypes
  Core
  MC
  SelectionDAG
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
