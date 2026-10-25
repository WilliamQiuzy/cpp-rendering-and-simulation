# stb_image_write v1.16 (public domain or MIT), https://github.com/nothings/stb
add_library(stb_image_write STATIC stb_image_write.c)
target_include_directories(stb_image_write SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(stb_image_write PRIVATE $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-w>)
