add_llvm_component_library(LLVMAVMDisassembler
  AVMDisassembler.cpp

  DEPENDS
  AVMCommonTableGen

  LINK_COMPONENTS
  AVMDesc
  AVMInfo
  MC
  MCDisassembler
  Support

  ADD_TO_COMPONENT
  AVM
)
