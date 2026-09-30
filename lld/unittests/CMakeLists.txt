function(add_lld_unittests test_dirname)
  add_unittest(LLDUnitTests ${test_dirname} ${ARGN})
endfunction()

if(NOT LLD_BUILD_ELF_ONLY)
  add_subdirectory(AsLibAll)
endif()
add_subdirectory(AsLibELF)
