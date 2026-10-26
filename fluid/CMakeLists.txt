# Fluid core (power diagrams, optimal transport, time stepping) shared by the CLI and the tests.
add_library(photonflow_fluid STATIC
  src/fluid.cpp
  src/frame_writer.cpp
  src/optimal_transport.cpp
  src/polygon.cpp
  src/power_diagram.cpp
)
target_include_directories(photonflow_fluid PUBLIC src)
target_link_libraries(photonflow_fluid PUBLIC lbfgs PRIVATE stb_image_write)
target_compile_options(photonflow_fluid PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)
if(OpenMP_CXX_FOUND)
  target_link_libraries(photonflow_fluid PUBLIC OpenMP::OpenMP_CXX)
endif()

add_executable(photonflow-fluid src/main.cpp)
target_link_libraries(photonflow-fluid PRIVATE photonflow_fluid)
target_compile_options(photonflow-fluid PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)
