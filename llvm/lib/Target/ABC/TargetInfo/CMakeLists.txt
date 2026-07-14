add_llvm_component_library(LLVMABCInfo
  ABCTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  ABC
  )
