add_llvm_component_library(LLVMAVMAsmParser
  AVMAsmParser.cpp

  DEPENDS
  AVMCommonTableGen

  LINK_COMPONENTS
  AVMDesc
  AVMInfo
  MC
  MCParser
  Support

  ADD_TO_COMPONENT
  AVM
)
