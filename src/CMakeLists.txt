add_library(plugintercept SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debug_agent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debug_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code_object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code_object_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code_object_recorder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code_object_swapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/trap_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger/logger.cpp)

include(ExternalProject)

ExternalProject_Add(extern_crcpp
    SOURCE_DIR        ${PROJECT_BINARY_DIR}/crcpp
    GIT_REPOSITORY    https://github.com/d-bahr/CRCpp
    GIT_TAG           534c1d8c5517cfbb0a0f1ff0d9ec4c8b8ffd78e2
    CONFIGURE_COMMAND ""
    UPDATE_COMMAND    ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ""
    LOG_DOWNLOAD      ON
)
ExternalProject_Get_Property(extern_crcpp SOURCE_DIR)
set(CRCPP_INCLUDE_DIR ${SOURCE_DIR}/inc)

ExternalProject_Add(extern_cpptoml
    SOURCE_DIR        ${PROJECT_BINARY_DIR}/cpptoml
    GIT_REPOSITORY    https://github.com/skystrife/cpptoml
    GIT_TAG           v0.1.1
    CONFIGURE_COMMAND ""
    UPDATE_COMMAND    ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ""
    LOG_DOWNLOAD      ON
)
ExternalProject_Get_Property(extern_cpptoml SOURCE_DIR)
set(CPPTOML_INCLUDE_DIR ${SOURCE_DIR}/include)

add_dependencies(plugintercept extern_crcpp extern_cpptoml)
target_link_libraries(plugintercept ${HSA_LIBRARY} ${HSAKMT_LIBRARY})
target_include_directories(plugintercept PUBLIC
    ${HSA_HEADER_DIR}
    ${ROCM_HEADER_DIR}
    ${CRCPP_INCLUDE_DIR}
    ${CPPTOML_INCLUDE_DIR})
