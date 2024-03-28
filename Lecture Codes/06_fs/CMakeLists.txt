add_executable(fuse_hello fuse_hello.c)
target_link_libraries(fuse_hello PRIVATE fuse3)
target_compile_definitions(fuse_hello PRIVATE _FILE_OFFSET_BITS=64)
