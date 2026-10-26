add_executable(test_render test_render.cpp)
target_link_libraries(test_render PRIVATE photonflow_render)
target_compile_definitions(test_render PRIVATE PHOTONFLOW_TEST_TMP="${CMAKE_CURRENT_BINARY_DIR}")

add_executable(test_fluid test_fluid.cpp)
target_link_libraries(test_fluid PRIVATE photonflow_fluid)

add_test(NAME render COMMAND test_render)
add_test(NAME fluid COMMAND test_fluid)

# End-to-end smoke tests of the two command-line programs.
add_test(NAME render_cli COMMAND photonflow-render --samples 2 --size 64 --glass-sphere --mirror-sphere
         --output ${CMAKE_CURRENT_BINARY_DIR}/smoke.png)
add_test(NAME fluid_cli COMMAND photonflow-fluid --particles 20 --frames 3
         --output-dir ${CMAKE_CURRENT_BINARY_DIR}/smoke_frames)
