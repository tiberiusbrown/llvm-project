add_lldb_library(lldbPluginABIAVM PLUGIN
  ABIAVM.cpp
  LINK_COMPONENTS
    Support
    TargetParser
  LINK_LIBS
    lldbCore
    lldbSymbol
    lldbTarget
  )
