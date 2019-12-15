#include <catch2/catch.hpp>
#include "../src/external_command.hpp"

using namespace agent;

TEST_CASE("captures stdout and stderr", "[external_cmd]")
{
  ExternalCommand cmd("echo Test test test; >&2 echo Error error error");
  REQUIRE(cmd.execute() == 0);
  REQUIRE(cmd.read_stdout() == "Test test test\n");
  REQUIRE(cmd.read_stderr() == "Error error error\n");
}

TEST_CASE("handles shell errors", "[external_cmd]")
{
  ExternalCommand cmd("asdfasdf");
  REQUIRE(cmd.execute() != 0);
  REQUIRE(cmd.read_stdout() == "");
  REQUIRE(cmd.read_stderr() == "sh: 1: asdfasdf: not found\n");
}
