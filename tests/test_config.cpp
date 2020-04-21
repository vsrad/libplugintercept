#include "../src/config/config.hpp"
#include <catch2/catch.hpp>

TEST_CASE("reads a valid configuration file", "[config]")
{
    agent::config::Config config;
    REQUIRE(config.agent_log_file() == "-");
    REQUIRE(config.code_object_log_file() == "tests/tmp/co_dump.log");
    REQUIRE(config.code_object_dump_dir() == "tests/tmp");

    std::vector<agent::config::Buffer> expected_buffers = {
        {.size = 1048576,
         .dump_path = "tests/tmp/debug_buffer",
         .addr_env_name = "ASM_DBG_BUF_ADDR",
         .size_env_name = "ASM_DBG_BUF_SIZE"},
        {.size = 4096,
         .dump_path = {},
         .addr_env_name = "ASM_HID_BUF_ADDR"}};
    REQUIRE(config.buffers() == expected_buffers);

    agent::config::InitCommand expected_init_command = {
        .command = "bash -o pipefail -c '"
                   "perl tests/fixtures/breakpoint_trap.pl -ba $ASM_DBG_BUF_ADDR -bs $ASM_DBG_BUF_SIZE -ha $ASM_HID_BUF_ADDR "
                   "-w v[tid_dump] -e \"s_nop 10\" -l 41 -t 2 tests/kernels/dbg_kernel.s | "
                   "/opt/rocm/bin/hcc -x assembler -target amdgcn--amdhsa "
                   "-mcpu=`/opt/rocm/bin/rocminfo | grep -om1 gfx9..` -mno-code-object-v3 "
                   "-Itests/kernels/include -o tests/tmp/replacement.co -'",
        .expect_retcode = 0};
    REQUIRE(config.init_command() == expected_init_command);

    agent::config::TrapHandler expected_trap_handler = {
        .code_object_path = "tests/tmp/replacement.co",
        .symbol_name = "trap_handler"};
    REQUIRE(config.trap_handler() == expected_trap_handler);

    std::vector<agent::config::CodeObjectSubstitute> expected_subs = {
        {.condition_crc = {},
         .condition_load_id = 1,
         .replacement_path = "tests/tmp/replacement.co"}};
    REQUIRE(config.code_object_subs() == expected_subs);

    std::vector<agent::config::CodeObjectSymbolSubstitute> expected_symbol_subs = {
        {.condition_crc = 0xCAFE666,
         .condition_load_id = 5,
         .condition_get_info_id = {},
         .condition_name = "conv2d",
         .replacement_name = "conv2d_test_new",
         .replacement_path = "replacement.co"},
        {.condition_crc = {},
         .condition_load_id = {},
         .condition_get_info_id = 1,
         .condition_name = {},
         .replacement_name = "conv2d_transpose_new",
         .replacement_path = "replacement.co"}};
    REQUIRE(config.symbol_subs() == expected_symbol_subs);
}

TEST_CASE("reads a minimal configuration file", "[config]")
{
    auto old_config = getenv("INTERCEPT_CONFIG");
    setenv("INTERCEPT_CONFIG", "tests/fixtures/minimal.toml", 1);
    agent::config::Config config;
    setenv("INTERCEPT_CONFIG", old_config, 1);

    REQUIRE(config.agent_log_file() == "tests/tmp/agent.log");
    REQUIRE(config.code_object_log_file() == "tests/tmp/co.log");
    REQUIRE(config.code_object_dump_dir().empty());
    REQUIRE(config.symbol_subs().empty());
    REQUIRE(config.buffers().empty());
}
