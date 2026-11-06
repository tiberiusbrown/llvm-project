get_property(LLDB_ALL_PLUGINS GLOBAL PROPERTY LLDB_PLUGINS)

add_lldb_tool(lldb-test
  FormatUtil.cpp
  lldb-test.cpp
  SystemInitializerTest.cpp

  LINK_COMPONENTS
    Support
  LINK_LIBS
    lldbBreakpoint
    lldbCore
    lldbDataFormatters
    lldbExpression
    lldbHost
    lldbInitialization
    lldbInterpreter
    lldbSymbol
    lldbTarget
    lldbUtility
    lldbVersion
    ${LLDB_ALL_PLUGINS}
  )

if(TARGET lldbPluginProcessAVM)
  # AVM profile symbolization uses the public SB API as well as private LLDB
  # libraries. Match other tools that link both without symbol interposition.
  target_link_libraries(lldb-test PRIVATE liblldb)
  lldb_prevent_liblldb_symbol_interposition(lldb-test)
endif()

if(Python3_RPATH)
  set_property(TARGET lldb-test APPEND PROPERTY INSTALL_RPATH "${Python3_RPATH}")
  set_property(TARGET lldb-test APPEND PROPERTY BUILD_RPATH   "${Python3_RPATH}")
endif()

target_include_directories(lldb-test PRIVATE ${LLDB_SOURCE_DIR}/source)
target_include_directories(lldb-test PRIVATE ${LLDB_BINARY_DIR}/source)
