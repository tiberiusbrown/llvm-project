add_llvm_component_library(LLVMAVMDesc
  AVMAsmBackend.cpp
  AVMInstPrinter.cpp
  AVMMCCodeEmitter.cpp
  AVMMCTargetDesc.cpp

  DEPENDS
  AVMCommonTableGen

  LINK_COMPONENTS
  AVMInfo
  MC
  MCDisassembler
  Support
  TargetParser

  ADD_TO_COMPONENT
  AVM
)
