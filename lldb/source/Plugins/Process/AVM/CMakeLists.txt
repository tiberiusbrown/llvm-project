add_lldb_library(lldbPluginProcessAVM PLUGIN
  ProcessAVM.cpp

  LINK_LIBS
    avm_debug_emulator
    lldbCore
    lldbTarget
    lldbUtility
  LINK_COMPONENTS
    Support
  )
if(MSVC)
  target_compile_options(lldbPluginProcessAVM PRIVATE /EHsc)
else()
  target_compile_options(lldbPluginProcessAVM PRIVATE -fexceptions)
endif()
