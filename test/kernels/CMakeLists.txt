set(ASM_CLANG_FLAGS -x assembler -target amdgcn--amdhsa -mcpu=gfx900 -mno-code-object-v3)
set(ASM_CLANG_CMD ${CLANG} ${ASM_CLANG_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}/include)
add_custom_target(dbg_kernel
    COMMAND ${ASM_CLANG_CMD} ${CMAKE_CURRENT_SOURCE_DIR}/dbg_kernel.s -o dbg_kernel.co
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dbg_kernel.s)
