add_library(plugintercept SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DebugAgent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CodeObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CodeObjectManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CodeObjectSwapper.cpp)
add_subdirectory(logger)

target_include_directories(plugintercept PUBLIC ${HSA_HEADER_DIR} ${PROJECT_SOURCE_DIR}/external/CRCpp/inc)
target_link_libraries(plugintercept ${HSA_LIBRARY})
