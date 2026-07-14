add_lld_library(lldABC
  Driver.cpp

  LINK_COMPONENTS
  BinaryFormat
  Object
  Support
  TargetParser

  LINK_LIBS
  lldCommon

  DEPENDS
  intrinsics_gen
  )
