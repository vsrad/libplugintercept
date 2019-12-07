add_library(plugintercept SHARED init.cpp)
target_include_directories(plugintercept PUBLIC ${HSA_HEADER_DIR})
target_link_libraries(plugintercept ${HSA_LIBRARY})
