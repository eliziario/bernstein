function(bernstein_enable_static_analysis target)
  if(BERNSTEIN_ENABLE_CLANG_TIDY)
    find_program(
      BERNSTEIN_CLANG_TIDY
      NAMES clang-tidy
      HINTS "$ENV{BERNSTEIN_LLVM_BIN}" /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin REQUIRED
    )
    set_property(
      TARGET ${target}
      PROPERTY CXX_CLANG_TIDY
               "${BERNSTEIN_CLANG_TIDY};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
    )
  endif()
endfunction()
