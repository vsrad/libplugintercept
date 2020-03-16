add_executable(tests
  main.cpp
  test_config.cpp
  test_co_recorder.cpp
  test_co_swapper.cpp
  test_external_command.cpp
  test_integration.cpp
)

add_dependencies(tests plugintercept dbg_kernel)
add_subdirectory(kernels)

get_property(CATCH2_INCLUDE_DIR GLOBAL PROPERTY CATCH2_INCLUDE_DIR)

target_include_directories(tests PUBLIC ${HSA_HEADER_DIR} ${CATCH2_INCLUDE_DIR})
target_link_libraries(tests ${HSA_LIBRARY} plugintercept)

# put enable_testing() in the top-level CMakeLists.txt to enable CTest integration
# add_test(NAME tests COMMAND tests)
