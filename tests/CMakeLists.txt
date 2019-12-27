add_executable(tests
  main.cpp
  test_config.cpp
  test_co_swapper.cpp
  test_co_manager.cpp
  test_external_command.cpp
  test_integration.cpp
)

target_include_directories(tests PUBLIC ${HSA_HEADER_DIR})
target_link_libraries(tests Catch2::Catch2 ${HSA_LIBRARY} plugintercept)

add_dependencies(tests plugintercept dbg_kernel)
add_subdirectory(kernels)

get_property(CMAKE_MODULE_PATH GLOBAL PROPERTY CATCH2_MODULE_PATH)

include(CTest)
include(Catch)
catch_discover_tests(tests)

add_test(NAME tests COMMAND tests)
