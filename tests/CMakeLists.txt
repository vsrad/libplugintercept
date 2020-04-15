add_executable(tests
  main.cpp
  test_config.cpp
  test_co_recorder.cpp
  test_co_substitutor.cpp
  test_external_command.cpp
  test_integration.cpp
)

include(ExternalProject)
ExternalProject_Add(extern_catch2
    SOURCE_DIR        ${PROJECT_BINARY_DIR}/catch2
    GIT_REPOSITORY    https://github.com/catchorg/Catch2.git
    GIT_TAG           v2.11.0
    CONFIGURE_COMMAND ""
    UPDATE_COMMAND    ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ""
    LOG_DOWNLOAD      ON
)
ExternalProject_Get_Property(extern_catch2 SOURCE_DIR)
set(CATCH2_INCLUDE_DIR ${SOURCE_DIR}/single_include)

add_dependencies(tests plugintercept dbg_kernel extern_catch2)
add_subdirectory(kernels)

target_include_directories(tests PUBLIC ${HSA_HEADER_DIR} ${CATCH2_INCLUDE_DIR})
target_link_libraries(tests ${HSA_LIBRARY} plugintercept)

# put enable_testing() in the top-level CMakeLists.txt to enable CTest integration
# add_test(NAME tests COMMAND tests)
