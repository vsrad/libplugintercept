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

    agent::CodeObjectSwap with_symbols = {.condition = {crc32_t(0xCAFE666)}, .replacement_path = "replacement.co"};
    with_symbols.symbol_swaps.push_back({"conv2d", "conv2d_test_new"});
    with_symbols.symbol_swaps.push_back({"conv2d_transpose", "conv2d_test_new_transpose"});

    std::vector<agent::CodeObjectSwap> expected_swaps = {
        {.condition = {call_count_t(1)},
         .replacement_path = "tests/tmp/replacement.co",
         .trap_handler_path = "tests/tmp/replacement.co",
         .external_command = "bash -o pipefail -c '"
                             "perl tests/fixtures/breakpoint_trap.pl -ba $ASM_DBG_BUF_ADDR -bs $ASM_DBG_BUF_SIZE "
                             "-w v[tid_dump] -t 0 tests/kernels/dbg_kernel.s | "
                             "/opt/rocm/bin/hcc -x assembler -target amdgcn--amdhsa "
                             "-mcpu=`/opt/rocm/bin/rocminfo | grep -om1 gfx9..` -mno-code-object-v3 "
                             "-Itests/kernels/include -o tests/tmp/replacement.co -'"},
        with_symbols,
        {.condition = {crc32_t(0xDEADBEEF)},
         .replacement_path = "replacement.co"}};
    std::vector<agent::CodeObjectSwap> swaps = config.code_object_swaps();
    REQUIRE(swaps == expected_swaps);
}

TEST_CASE("reads a minimal configuration file", "[config]")
{
    auto old_config = getenv("ASM_DBG_CONFIG");
    setenv("ASM_DBG_CONFIG", "tests/fixtures/minimal.toml", 1);
    agent::Config config;
    setenv("ASM_DBG_CONFIG", old_config, 1);

    REQUIRE(config.agent_log_file() == "agent.log");
    REQUIRE(config.code_object_log_file() == "co.log");
    REQUIRE(config.code_object_dump_dir() == "/co/dump/dir");
    REQUIRE(config.code_object_swaps().empty());
    REQUIRE(config.debug_buffer_dump_file() == "");
    REQUIRE(config.debug_buffer_size() == 0);
}
