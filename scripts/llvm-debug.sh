#!/bin/bash -v
cd ../
cmake -S llvm -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DLLVM_ENABLE_PROJECTS="clang;lld;bolt" \
    -DLLVM_TARGETS_TO_BUILD="X86;AArch64" \
    -DLLVM_ENABLE_ASSERTIONS=ON
./scripts/build.sh
