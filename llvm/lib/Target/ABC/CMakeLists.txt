add_llvm_component_group(ABC)

set(LLVM_TARGET_DEFINITIONS ABC.td)

tablegen(LLVM ABCGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM ABCGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM ABCGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(ABCCommonTableGen)

add_llvm_target(ABCCodeGen
  ABCIRTranslator.cpp
  ABCAsmPrinter.cpp
  ABCTargetTransformInfo.cpp
  ABCFrameLowering.h
  ABCInstrInfo.cpp
  ABCRegisterInfo.cpp
  ABCSubtarget.cpp
  ABCTargetMachine.cpp

  LINK_COMPONENTS
  AsmPrinter
  CodeGen
  Core
  MC
  ABCDesc
  ABCInfo
  SelectionDAG
  Support
  Target
  TargetParser

  ADD_TO_COMPONENT
  ABC
  )

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
