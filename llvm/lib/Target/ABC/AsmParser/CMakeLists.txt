add_llvm_component_library(LLVMABCAsmParser
  ABCAsmParser.cpp

  LINK_COMPONENTS
  ABCDesc
  ABCInfo
  MC
  MCParser
  Support

  ADD_TO_COMPONENT
  ABC
  )
