add_executable(path_tracer
  src/main.cpp
  src/scene.cpp
  src/triangle_mesh.cpp
)
target_include_directories(path_tracer PRIVATE src)
target_link_libraries(path_tracer PRIVATE stb_image_write)
target_compile_definitions(path_tracer PRIVATE
  PATH_TRACER_DEFAULT_MODEL="${CMAKE_CURRENT_SOURCE_DIR}/assets/cat.obj")
target_compile_options(path_tracer PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)
if(OpenMP_CXX_FOUND)
  target_link_libraries(path_tracer PRIVATE OpenMP::OpenMP_CXX)
endif()
