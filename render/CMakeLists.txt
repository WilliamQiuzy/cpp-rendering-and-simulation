# Rendering core (geometry, BVH, shading) shared by the CLI and the tests.
add_library(photonflow_render STATIC
  src/scene.cpp
  src/triangle_mesh.cpp
)
target_include_directories(photonflow_render PUBLIC src)
target_compile_options(photonflow_render PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)

add_executable(photonflow-render src/main.cpp)
target_link_libraries(photonflow-render PRIVATE photonflow_render stb_image_write)
target_compile_definitions(photonflow-render PRIVATE
  PHOTONFLOW_DEFAULT_MODEL="${CMAKE_CURRENT_SOURCE_DIR}/assets/cat.obj")
target_compile_options(photonflow-render PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)
if(OpenMP_CXX_FOUND)
  target_link_libraries(photonflow-render PRIVATE OpenMP::OpenMP_CXX)
endif()
