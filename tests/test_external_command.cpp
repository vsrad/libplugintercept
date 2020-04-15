#include "../src/external_command.hpp"
#include "log_helper.hpp"
#include <catch2/catch.hpp>

using namespace agent;

TEST_CASE("captures stdout and stderr", "[external_cmd]")
{
    ExternalCommand cmd;
    REQUIRE(cmd.execute("echo Test test test; >&2 echo Error error error", {}) == 0);
    REQUIRE(cmd.read_stdout() == "Test test test\n");
    REQUIRE(cmd.read_stderr() == "Error error error\n");
}

TEST_CASE("handles shell errors", "[external_cmd]")
{
    ExternalCommand cmd;
    REQUIRE(cmd.execute("asdfasdf", {}) != 0);
    REQUIRE(cmd.read_stdout() == "");
    REQUIRE(cmd.read_stderr() == "sh: 1: asdfasdf: not found\n");
}

TEST_CASE("sets environment variables", "[external_cmd]")
{
    setenv("EXT_TEST_VAR1", "some external value (fails the test if printed)", 1);
    ExternalCommand cmd;
    ext_environment_t env = {{"EXT_TEST_VAR1", "long var"}, {"EXT_TEST_VAR2", "42"}};
    REQUIRE(cmd.execute("echo $EXT_TEST_VAR1 $EXT_TEST_VAR2", env) == 0);
    REQUIRE(cmd.read_stdout() == "long var 42\n");
    REQUIRE(cmd.read_stderr() == "");
}

TEST_CASE("run_init_command throws an exception on failure", "[external_cmd]")
{
    TestLogger logger;

    ExternalCommand::run_init_command(config::InitCommand{"asdfasdf", 32512}, {{"VAR", "hh"}}, logger);
    REQUIRE(logger.errors.empty());

    using Catch::Matchers::EndsWith;

    REQUIRE_THROWS_WITH(ExternalCommand::run_init_command(config::InitCommand{"asdfasdf", 0}, {}, logger), EndsWith("failed with code 32512"));
    std::vector<std::string> expected_error_log = {
        "The command `asdfasdf` has exited with code 32512 (expected 0)"
        "\n=== Stderr:\nsh: 1: asdfasdf: not found\n"};
    REQUIRE(logger.errors == expected_error_log);
    logger.errors.clear();

    REQUIRE_THROWS_WITH(ExternalCommand::run_init_command(config::InitCommand{"echo $VAR; asdfasdf", 0}, {{"VAR", "hh"}}, logger), EndsWith("failed with code 32512"));
    expected_error_log = {
        "The command `echo $VAR; asdfasdf` has exited with code 32512 (expected 0)"
        "\n=== Stdout:\nhh\n"
        "\n=== Stderr:\nsh: 1: asdfasdf: not found\n"};
    REQUIRE(logger.errors == expected_error_log);
}
