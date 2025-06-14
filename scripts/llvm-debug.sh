#!/bin/bash -v
cd ../
cmake -S llvm -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    # use `find /usr -name plugin-api.h` to get the directory, then you will get LLVMgold.so
    # see https://discourse.llvm.org/t/llvmgold-so-error-loading-plugin-cannot-open-shared-object-file-no-such-file-or-directory/86858
    -DLLVM_BINUTILS_INCDIR=/usr/local/gcc-10/gcc-10.3.0/include \
    -DLLVM_ENABLE_PROJECTS="clang;lld;bolt" \
    -DLLVM_TARGETS_TO_BUILD="X86;AArch64" \
    -DLLVM_ENABLE_ASSERTIONS=ON
./scripts/build.sh
