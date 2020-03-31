#include "../src/config.hpp"
#include <catch2/catch.hpp>

TEST_CASE("reads a valid configuration file", "[config]")
{
    agent::Config config;
    REQUIRE(config.agent_log_file() == "-");
    REQUIRE(config.code_object_log_file() == "tests/tmp/co_dump.log");
    REQUIRE(config.code_object_dump_dir() == "tests/tmp/");

    std::vector<agent::BufferAllocation> expected_allocs = {
        {.size = 1048576,
         .dump_path = "tests/tmp/debug_buffer",
         .addr_env_name = "ASM_DBG_BUF_ADDR",
         .size_env_name = "ASM_DBG_BUF_SIZE"},
        {.size = 4096,
         .dump_path = {},
         .addr_env_name = "ASM_HID_BUF_ADDR"}};
    REQUIRE(config.buffer_allocations() == expected_allocs);

    agent::TrapHandlerConfig expected_trap_handler = {
        .code_object_path = "tests/tmp/replacement.co",
        .symbol_name = "trap_handler",
        .external_command = "bash -o pipefail -c '"
                            "perl tests/fixtures/breakpoint_trap.pl -ba $ASM_DBG_BUF_ADDR -bs $ASM_DBG_BUF_SIZE -ha $ASM_HID_BUF_ADDR "
                            "-w v[tid_dump] -e \"s_nop 10\" -l 37 -t 2 tests/kernels/dbg_kernel.s | "
                            "/opt/rocm/bin/hcc -x assembler -target amdgcn--amdhsa "
                            "-mcpu=`/opt/rocm/bin/rocminfo | grep -om1 gfx9..` -mno-code-object-v3 "
                            "-Itests/kernels/include -o tests/tmp/replacement.co -'"};
    REQUIRE(config.trap_handler() == expected_trap_handler);

    std::vector<agent::CodeObjectSwap> expected_swaps = {
        {.condition = {.crc = {}, .load_call_id = 1},
         .replacement_path = "tests/tmp/replacement.co",
         .external_command = "bash -o pipefail -c '"
                             "perl tests/fixtures/breakpoint.pl -ba $ASM_DBG_BUF_ADDR -bs $ASM_DBG_BUF_SIZE "
                             "-w v[tid_dump] -l 37 -t 0 tests/kernels/dbg_kernel.s | "
                             "/opt/rocm/bin/hcc -x assembler -target amdgcn--amdhsa "
                             "-mcpu=`/opt/rocm/bin/rocminfo | grep -om1 gfx9..` -mno-code-object-v3 "
                             "-Itests/kernels/include -o tests/tmp/replacement.co -'"},
        {.condition = {.crc = {}, .load_call_id = 2},
         .replacement_path = "tests/tmp/replacement.co",
         .external_command = "bash -o pipefail -c '"
                             "perl tests/fixtures/breakpoint_trap.pl -ba $ASM_DBG_BUF_ADDR -bs $ASM_DBG_BUF_SIZE -ha $ASM_HID_BUF_ADDR "
                             "-w v[tid_dump] -e \"s_nop 10\" -l 37 -t 2 tests/kernels/dbg_kernel.s | "
                             "/opt/rocm/bin/hcc -x assembler -target amdgcn--amdhsa "
                             "-mcpu=`/opt/rocm/bin/rocminfo | grep -om1 gfx9..` -mno-code-object-v3 "
                             "-Itests/kernels/include -o tests/tmp/replacement.co -'"}};
    std::vector<agent::CodeObjectSwap> swaps = config.code_object_swaps();
    REQUIRE(swaps == expected_swaps);

    std::vector<agent::CodeObjectSymbolSubstitute> expected_symbol_subs = {
        {.condition = {.crc = 0xCAFE666, .load_call_id = 5},
         .source_name = "conv2d",
         .replacement_name = "conv2d_test_new",
         .replacement_path = "replacement.co",
         .external_command = "echo done"},
        {.condition = {},
         .source_name = "conv2d_transpose",
         .replacement_name = "conv2d_transpose_new",
         .replacement_path = "replacement.co"}};
    std::vector<agent::CodeObjectSymbolSubstitute> symbol_subs = config.code_object_symbol_subs();
    REQUIRE(symbol_subs == expected_symbol_subs);
}

TEST_CASE("reads a minimal configuration file", "[config]")
{
    auto old_config = getenv("INTERCEPT_CONFIG");
    setenv("INTERCEPT_CONFIG", "tests/fixtures/minimal.toml", 1);
    agent::Config config;
    setenv("INTERCEPT_CONFIG", old_config, 1);

    REQUIRE(config.agent_log_file() == "agent.log");
    REQUIRE(config.code_object_log_file() == "co.log");
    REQUIRE(config.code_object_dump_dir() == "/co/dump/dir");
    REQUIRE(config.code_object_swaps().empty());
    REQUIRE(config.buffer_allocations().empty());
}
