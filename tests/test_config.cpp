#include "../src/config.hpp"
#include <catch2/catch.hpp>

TEST_CASE("reads a valid configuration file", "[config]")
{
    agent::Config config;
    REQUIRE(config.debug_buffer_size() == 1024);
    REQUIRE(config.debug_buffer_dump_file() == "tmp/dump/buffer");
    REQUIRE(config.code_object_log_file() == "-");
    REQUIRE(config.code_object_dump_dir() == "tmp/dump/");

    std::vector<agent::CodeObjectSwap> expected_swaps = {
        {.condition = {crc32_t(0xDEADBEEF)},
         .replacement_path = "replacement.co",
         .external_command = "touch replacement.co"}};
    std::vector<agent::CodeObjectSwap> swaps = *config.code_object_swaps();
    REQUIRE(swaps == expected_swaps);
}
