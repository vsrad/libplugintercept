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
