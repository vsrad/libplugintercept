add_library(plugintercept SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DebugAgent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CodeObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CodeObjectManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CodeObjectSwapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external_command.cpp)
add_subdirectory(logger)

get_property(CRCPP_INCLUDE_DIR GLOBAL PROPERTY CRCPP_INCLUDE_DIR)

target_include_directories(plugintercept PUBLIC ${HSA_HEADER_DIR} ${CRCPP_INCLUDE_DIR})
target_link_libraries(plugintercept ${HSA_LIBRARY})
