add_library(allocctx SHARED ctx.cc)
target_compile_options(allocctx PRIVATE -g)

add_executable(bin bin.cc stub.cc)
target_compile_options(bin PRIVATE -g)
# linker lld/gold/mold is tested
# set(CMAKE_LINKER ld)
target_link_options(bin PRIVATE -Wl,-q -fuse-ld=lld)
# target_link_options(bin PRIVATE -Wl,-q -fuse-ld=gold)
# target_link_options(bin PRIVATE -Wl,-q -fuse-ld=mold)
target_link_libraries(bin PRIVATE allocctx)

add_executable(visitor visitor.cc stub.cc)
target_link_options(visitor PRIVATE -Wl,-q -fuse-ld=lld)
target_link_libraries(visitor PRIVATE allocctx)
