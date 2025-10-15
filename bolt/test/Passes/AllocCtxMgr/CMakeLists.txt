set(CMAKE_CXX_STANDARD 20)

add_library(allocctx SHARED ctx.cc ctx.s)
target_compile_options(allocctx PRIVATE -g -fPIC)

add_library(alloc SHARED alloc.cc)
target_compile_options(alloc PRIVATE -g -fPIC)
target_link_libraries(alloc PRIVATE allocctx)

add_library(demo_alloc SHARED demo_alloc.cc)
target_compile_options(demo_alloc PRIVATE -g -fPIC)
target_link_libraries(demo_alloc PRIVATE allocctx)

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

add_executable(shared_node_visitor shared_node_visitor.cc stub.cc)
target_link_options(shared_node_visitor PRIVATE -Wl,-q -fuse-ld=lld)
target_link_libraries(shared_node_visitor PRIVATE allocctx)
