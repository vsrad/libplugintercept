add_library(plugintercept SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DebugAgent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CodeObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CodeObjectManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CodeObjectSwapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code_object_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger/logger.cpp)

get_property(CRCPP_INCLUDE_DIR GLOBAL PROPERTY CRCPP_INCLUDE_DIR)
get_property(CPPTOML_INCLUDE_DIR GLOBAL PROPERTY CPPTOML_INCLUDE_DIR)

target_include_directories(plugintercept PUBLIC ${HSA_HEADER_DIR} ${CRCPP_INCLUDE_DIR} ${CPPTOML_INCLUDE_DIR})
target_link_libraries(plugintercept ${HSA_LIBRARY})
