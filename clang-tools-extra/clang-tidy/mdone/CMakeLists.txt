set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

add_clang_library(clangTidyMDOneModule
  avoidpragmaoncecheck.cpp
  mdonemodule.cpp


  LINK_LIBS
  clangTidy
  clangTidyUtils

  DEPENDS
  omp_gen
  ClangDriverOptions
  )

clang_target_link_libraries(clangTidyGoogleModule
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  )