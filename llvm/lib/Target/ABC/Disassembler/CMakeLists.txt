add_llvm_component_library(LLVMABCDisassembler
  ABCDisassembler.cpp

  LINK_COMPONENTS
  ABCDesc
  ABCInfo
  MC
  MCDisassembler
  Support

  ADD_TO_COMPONENT
  ABC
  )
