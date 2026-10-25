# liblbfgs by Naoaki Okazaki (MIT, see COPYING), vendored from
# https://github.com/chokkan/liblbfgs at commit 5ad02fbefefdeff339ab03635e673571055a0644.
add_library(lbfgs STATIC lbfgs.c)
target_include_directories(lbfgs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(lbfgs PROPERTIES C_STANDARD 99)
