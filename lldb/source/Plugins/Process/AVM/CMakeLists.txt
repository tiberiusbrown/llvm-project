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
target_include_directories(lldbPluginProcessAVM PRIVATE
  "${CMAKE_SOURCE_DIR}/debug")
if(MSVC)
  target_compile_options(lldbPluginProcessAVM PRIVATE /EHsc)
else()
  target_compile_options(lldbPluginProcessAVM PRIVATE -fexceptions)
endif()
