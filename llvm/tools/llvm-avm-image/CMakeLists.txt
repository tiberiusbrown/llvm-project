set(LLVM_LINK_COMPONENTS
  BinaryFormat
  DebugInfoDWARF
  Demangle
  Object
  Support
)

add_llvm_tool(llvm-avm-image
  llvm-avm-image.cpp
)

target_link_libraries(llvm-avm-image PRIVATE ZLIB::ZLIB)
add_dependencies(llvm-avm-image zlibstatic)
