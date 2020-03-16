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
set_property(GLOBAL PROPERTY CRCPP_INCLUDE_DIR ${SOURCE_DIR}/inc)

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
set_property(GLOBAL PROPERTY CPPTOML_INCLUDE_DIR ${SOURCE_DIR}/include)

ExternalProject_Add(extern_catch2
    SOURCE_DIR        ${PROJECT_BINARY_DIR}/catch2
    GIT_REPOSITORY    https://github.com/catchorg/Catch2.git
    GIT_TAG           v2.11.0
    CONFIGURE_COMMAND ""
    UPDATE_COMMAND    ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ""
    LOG_DOWNLOAD      ON
)
ExternalProject_Get_Property(extern_catch2 SOURCE_DIR BINARY_DIR)
set_property(GLOBAL PROPERTY CATCH2_INCLUDE_DIR ${SOURCE_DIR}/single_include)
