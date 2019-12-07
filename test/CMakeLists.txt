add_executable(testexe main.cpp)
target_include_directories(testexe PUBLIC ${HSA_HEADER_DIR})
target_link_libraries(testexe ${HSA_LIBRARY})

add_dependencies(testexe dbg_kernel)
add_subdirectory(kernels)
