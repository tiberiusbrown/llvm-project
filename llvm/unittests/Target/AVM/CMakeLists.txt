set(LLVM_LINK_COMPONENTS
  AVM
  Support
  )

include_directories(
  ${LLVM_MAIN_SRC_DIR}/lib/Target/AVM
  )

add_llvm_target_unittest(AVMCostModelTests
  AVMCostModelTest.cpp
  )
