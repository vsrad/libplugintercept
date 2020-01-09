#include "../src/config.hpp"
#include <catch2/catch.hpp>

TEST_CASE("reads a valid configuration file", "[config]")
{
    agent::Config config;
    REQUIRE(config.agent_log_file() == "-");
    REQUIRE(config.debug_buffer_size() == 1048576);
    REQUIRE(config.debug_buffer_dump_file() == "tests/tmp/debug_buffer");
    REQUIRE(config.code_object_log_file() == "tests/tmp/co_dump.log");
    REQUIRE(config.code_object_dump_dir() == "tests/tmp/");

    std::vector<agent::CodeObjectSwap> expected_swaps = {
        {.condition = {crc32_t(0xDEADBEEF)},
         .replacement_path = "replacement.co"},
        {.condition = {call_count_t(1)},
         .replacement_path = "tests/tmp/replacement.co",
         .external_command = "bash -o pipefail -c '"
                             "perl tests/fixtures/breakpoint.pl -ba $ASM_DBG_BUF_ADDR -bs $ASM_DBG_BUF_SIZE "
                             "-l 31 -w v[tid] -s 96 -r s0 -t 0 tests/kernels/dbg_kernel.s | "
                             "/opt/rocm/opencl/bin/x86_64/clang -x assembler -target amdgcn--amdhsa -mcpu=gfx900 -mno-code-object-v3 "
                             "-Itests/kernels/include -o tests/tmp/replacement.co -'"}};
    std::vector<agent::CodeObjectSwap> swaps = *config.code_object_swaps();
    REQUIRE(swaps == expected_swaps);
}
