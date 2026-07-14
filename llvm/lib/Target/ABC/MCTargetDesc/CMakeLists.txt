add_llvm_component_library(LLVMABCDesc
  ABCAsmBackend.cpp
  ABCELFObjectWriter.cpp
  ABCInstPrinter.cpp
  ABCMCAsmInfo.cpp
  ABCMCCodeEmitter.cpp
  ABCMCTargetDesc.cpp

  LINK_COMPONENTS
  ABCInfo
  BinaryFormat
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  ABC
  )
