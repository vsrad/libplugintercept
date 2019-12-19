cmake_minimum_required(VERSION 3.14)
include(FetchContent)

FetchContent_Declare(
  extern_crcpp
  GIT_REPOSITORY https://github.com/d-bahr/CRCpp
  GIT_TAG        534c1d8c5517cfbb0a0f1ff0d9ec4c8b8ffd78e2
)
FetchContent_GetProperties(extern_crcpp)
if(NOT extern_crcpp_POPULATED)
  FetchContent_Populate(extern_crcpp)
  set_property(GLOBAL PROPERTY CRCPP_INCLUDE_DIR ${extern_crcpp_SOURCE_DIR}/inc)
endif()

FetchContent_Declare(
  extern_cpptoml
  GIT_REPOSITORY https://github.com/skystrife/cpptoml
  GIT_TAG        v0.1.1
)
FetchContent_GetProperties(extern_cpptoml)
if(NOT extern_cpptoml_POPULATED)
  FetchContent_Populate(extern_cpptoml)
  set_property(GLOBAL PROPERTY CPPTOML_INCLUDE_DIR ${extern_cpptoml_SOURCE_DIR}/include)
endif()

FetchContent_Declare(
  extern_catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.11.0
)
FetchContent_GetProperties(extern_catch2)
if(NOT extern_catch2_POPULATED)
  FetchContent_Populate(extern_catch2)
  add_subdirectory(${extern_catch2_SOURCE_DIR} ${extern_catch2_BINARY_DIR})
  set_property(GLOBAL PROPERTY CATCH2_MODULE_PATH ${extern_catch2_SOURCE_DIR}/contrib)
endif()
