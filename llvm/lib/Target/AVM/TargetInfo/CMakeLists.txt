add_llvm_component_library(LLVMAVMInfo
  AVMTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  AVM
)
