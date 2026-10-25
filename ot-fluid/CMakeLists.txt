add_executable(ot_fluid
  src/main.cpp
  src/fluid.cpp
  src/frame_writer.cpp
  src/optimal_transport.cpp
  src/polygon.cpp
  src/power_diagram.cpp
)
target_include_directories(ot_fluid PRIVATE src)
target_link_libraries(ot_fluid PRIVATE lbfgs stb_image_write)
target_compile_options(ot_fluid PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)
if(OpenMP_CXX_FOUND)
  target_link_libraries(ot_fluid PRIVATE OpenMP::OpenMP_CXX)
endif()
