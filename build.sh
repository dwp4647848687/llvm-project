cd build &&
cmake -G "Ninja" \
  -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" \
  -DCMAKE_BUILD_TYPE=Release \
  ../llvm

ninja clang-tidy