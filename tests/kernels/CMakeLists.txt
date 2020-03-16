execute_process(
    COMMAND bash "-c" "/opt/rocm/bin/rocminfo | grep -om1 gfx9.."
    OUTPUT_VARIABLE GFX_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
set(ASM_COMPILE_FLAGS -x assembler -target amdgcn--amdhsa -mcpu=${GFX_VERSION} -mno-code-object-v3)
set(ASM_COMPILE_CMD ${HCC} ${ASM_COMPILE_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}/include)

add_custom_target(dbg_kernel
    COMMAND ${ASM_COMPILE_CMD} ${CMAKE_CURRENT_SOURCE_DIR}/dbg_kernel.s -o dbg_kernel.co
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dbg_kernel.s)
