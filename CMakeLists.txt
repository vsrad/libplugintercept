cmake_minimum_required(VERSION 3.14)

find_program(CLANG NAMES clang REQUIRED)
if(CLANG)
    message(STATUS "Found clang: ${CLANG}")
else()
    MESSAGE(FATAL_ERROR "clang not found")
endif()

set(HSA_ROOT /opt/rocm/hsa CACHE PATH "HSA runtime path")
set(HSA_HEADER_DIR ${HSA_ROOT}/include/hsa CACHE PATH "HSA include path")
set(HSA_LIBRARY_DIR ${HSA_ROOT}/lib CACHE PATH "HSA library path")

find_path(HSA_HEADER_DIR hsa.h PATHS ${HSA_HEADER_DIR} NO_DEFAULT_PATH)
if(HSA_HEADER_DIR)
  MESSAGE(STATUS "Found HSA header: ${HSA_HEADER_DIR}/hsa.h")
else(HSA_HEADER_DIR)
  MESSAGE(FATAL_ERROR "HSA header not found (searched ${HSA_HEADER_DIR})")
endif(HSA_HEADER_DIR)

include_directories(${HSA_HEADER_DIR})

find_library(HSA_LIBRARY libhsa-runtime64.so.1 PATHS ${HSA_LIBRARY_DIR} NO_DEFAULT_PATH)
if(HSA_LIBRARY)
  MESSAGE(STATUS "Found HSA library: ${HSA_LIBRARY}")
else(HSA_LIBRARY)
  MESSAGE(FATAL_ERROR "HSA runtime library not found (searched ${HSA_LIBRARY_DIR})")
endif(HSA_LIBRARY)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -g -fPIC")

add_subdirectory(extern)
add_subdirectory(src)
add_subdirectory(tests)
