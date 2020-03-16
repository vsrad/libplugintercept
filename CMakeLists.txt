cmake_minimum_required(VERSION 3.5)
project(debug-plug-hsa-intercept VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
add_compile_options("-Wall" "-g")

set(HSA_ROOT /opt/rocm/hsa CACHE PATH "HSA runtime path")
set(HSA_HEADER_DIR ${HSA_ROOT}/include/hsa CACHE PATH "HSA include path")
set(HSA_LIBRARY_DIR ${HSA_ROOT}/lib CACHE PATH "HSA library path")

set(ROCM_ROOT /opt/rocm CACHE PATH "ROCm installation path")
set(ROCM_HEADER_DIR ${ROCM_ROOT}/include CACHE PATH "ROCm include path")
set(ROCM_LIBRARY_DIR ${ROCM_ROOT}/lib CACHE PATH "ROCm library path")

find_program(HCC NAMES hcc HINTS ${ROCM_ROOT}/bin REQUIRED)
if(HCC)
    message(STATUS "Found HCC: ${HCC}")
else()
    message(FATAL_ERROR "HCC not found")
endif()

find_path(HSA_HEADER_DIR hsa.h PATHS ${HSA_HEADER_DIR} NO_DEFAULT_PATH)
if(HSA_HEADER_DIR)
  message(STATUS "Found HSA header: ${HSA_HEADER_DIR}/hsa.h")
else(HSA_HEADER_DIR)
  message(FATAL_ERROR "HSA header not found (searched ${HSA_HEADER_DIR})")
endif(HSA_HEADER_DIR)

find_path(ROCM_HEADER_DIR hsakmt.h PATHS ${ROCM_HEADER_DIR} NO_DEFAULT_PATH)
if(ROCM_HEADER_DIR)
  message(STATUS "Found HSAKMT header: ${ROCM_HEADER_DIR}/hsakmt.h")
else(ROCM_HEADER_DIR)
  message(FATAL_ERROR "HSAKMT header not found (searched ${ROCM_HEADER_DIR})")
endif(ROCM_HEADER_DIR)

include_directories(BEFORE ${HSA_HEADER_DIR})
include_directories(${ROCM_HEADER_DIR}) # hsa.h from ${ROCM_HEADER_DIR} should _not_ be included

find_library(HSA_LIBRARY libhsa-runtime64.so.1 PATHS ${HSA_LIBRARY_DIR} NO_DEFAULT_PATH)
if(HSA_LIBRARY)
  message(STATUS "Found HSA library: ${HSA_LIBRARY}")
else(HSA_LIBRARY)
  message(FATAL_ERROR "HSA runtime library not found (searched ${HSA_LIBRARY_DIR})")
endif(HSA_LIBRARY)

find_library(HSAKMT_LIBRARY libhsakmt.so PATHS ${ROCM_LIBRARY_DIR} NO_DEFAULT_PATH)
if(HSAKMT_LIBRARY)
  message(STATUS "Found HSAKMT library: ${HSAKMT_LIBRARY}")
else(HSAKMT_LIBRARY)
  message(FATAL_ERROR "HSAKMT runtime library not found (searched ${ROCM_LIBRARY_DIR})")
endif(HSAKMT_LIBRARY)

add_subdirectory(src)
add_subdirectory(tests)
