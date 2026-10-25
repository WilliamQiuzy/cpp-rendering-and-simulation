cmake_minimum_required(VERSION 3.16)
project(cpp_rendering_and_simulation LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# OpenMP is optional: both programs run single-threaded without it.
# On macOS with Apple Clang, install libomp (brew install libomp) and pass
# -DOpenMP_ROOT=$(brew --prefix libomp).
find_package(OpenMP COMPONENTS CXX)

add_subdirectory(third_party/stb)
add_subdirectory(third_party/liblbfgs)
add_subdirectory(path-tracer)
add_subdirectory(ot-fluid)
