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
