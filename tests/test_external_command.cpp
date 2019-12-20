#include "../src/external_command.hpp"
#include <catch2/catch.hpp>

using namespace agent;

TEST_CASE("captures stdout and stderr", "[external_cmd]")
{
    ExternalCommand cmd("echo Test test test; >&2 echo Error error error");
    REQUIRE(cmd.execute(std::map<std::string, std::string>()) == 0);
    REQUIRE(cmd.read_stdout() == "Test test test\n");
    REQUIRE(cmd.read_stderr() == "Error error error\n");
}

TEST_CASE("handles shell errors", "[external_cmd]")
{
    ExternalCommand cmd("asdfasdf");
    REQUIRE(cmd.execute(std::map<std::string, std::string>()) != 0);
    REQUIRE(cmd.read_stdout() == "");
    REQUIRE(cmd.read_stderr() == "sh: 1: asdfasdf: not found\n");
}

TEST_CASE("sets environment variables", "[external_cmd]")
{
    setenv("EXT_TEST_VAR_1", "some external value (fails the test if printed)", 1);
    ExternalCommand cmd("echo $EXT_TEST_VAR_1 $EXT_TEST_VAR_2");
    std::map<std::string, std::string> env = {{"EXT_TEST_VAR_1", "long var"}, {"EXT_TEST_VAR_2", "42"}};
    REQUIRE(cmd.execute(env) == 0);
    REQUIRE(cmd.read_stdout() == "long var 42\n");
    REQUIRE(cmd.read_stderr() == "");
}
