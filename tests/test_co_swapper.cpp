#include "../src/CodeObjectSwapper.hpp"
#include <catch2/catch.hpp>
#include <ctime>

using namespace agent;

struct TestLogger : Logger
{
    TestLogger() : infos(), errors() {}
    std::vector<std::string> infos;
    std::vector<std::string> errors;
    virtual void info(const std::string& msg) { infos.push_back(msg); }
    virtual void error(const std::string& msg) { errors.push_back(msg); }
};

TEST_CASE("swaps code object based on CRC match", "[co_swapper]")
{
    CodeObject co_matching("CODE OBJECT", sizeof("CODE OBJECT"));
    CodeObject co_other("SOME OTHER CO", sizeof("SOME OTHER CO"));
    REQUIRE(co_matching.CRC() != co_other.CRC());

    CodeObjectSwapper cosw(
        {{.condition = {co_matching.CRC()},
          .replacement_path = "tests/fixtures/asdf"}},
        std::make_shared<TestLogger>());
    auto swapped_matching = cosw.get_swapped_code_object(co_matching);
    auto swapped_other = cosw.get_swapped_code_object(co_other);
    REQUIRE(swapped_matching);
    REQUIRE(!swapped_other);
    std::string swapped_data(static_cast<const char*>(swapped_matching->Ptr()), swapped_matching->Size());
    REQUIRE(swapped_data == "ASDF\n");
}

TEST_CASE("swaps code object based on the call # match", "[co_swapper]")
{
    CodeObjectSwapper cosw(
        {{.condition = {call_count_t(3)},
          .replacement_path = "tests/fixtures/asdf"}},
        std::make_shared<TestLogger>());
    REQUIRE(!cosw.get_swapped_code_object(CodeObject("", 0)));
    REQUIRE(!cosw.get_swapped_code_object(CodeObject("", 0)));
    auto third_call = cosw.get_swapped_code_object(CodeObject("", 0));
    REQUIRE(third_call);
    std::string swapped_data(static_cast<const char*>(third_call->Ptr()), third_call->Size());
    REQUIRE(swapped_data == "ASDF\n");
}

TEST_CASE("runs external command before swapping the code object if specified", "[co_swapper]")
{
    auto time = std::time(0);
    auto logger = std::make_shared<TestLogger>();
    CodeObjectSwapper cosw(
        {{.condition = {call_count_t(1)},
          .replacement_path = "tests/fixtures/co_swapper_time",
          .external_command = "echo " + std::to_string(time) + " > tests/fixtures/co_swapper_time"}},
        logger);
    auto swapped = cosw.get_swapped_code_object(CodeObject("", 0));
    REQUIRE(swapped);
    std::string swapped_data(static_cast<const char*>(swapped->Ptr()), swapped->Size());
    REQUIRE(swapped_data == std::to_string(time) + "\n");
}

TEST_CASE("logs external command output on failure", "[co_swapper]")
{
    auto logger = std::make_shared<TestLogger>();
    CodeObjectSwapper cosw(
        {{.condition = {call_count_t(1)},
          .replacement_path = "tests/fixtures/asdf",
          .external_command = "asdfasdf"},
         {.condition = {call_count_t(2)},
          .replacement_path = "t",
          .external_command = "echo h; asdfasdf"}},
        logger);
    auto swapped = cosw.get_swapped_code_object(CodeObject("SOME CO", sizeof("SOME CO")));
    REQUIRE(!swapped);

    std::vector<std::string> expected_error_log = {
        "The command `asdfasdf` has exited with code 32512",
        "=== Stderr:\nsh: 1: asdfasdf: not found\n"};
    REQUIRE(logger->errors == expected_error_log);

    logger->errors.clear();
    auto swapped2 = cosw.get_swapped_code_object(CodeObject("SOME CO", sizeof("SOME CO")));
    REQUIRE(!swapped2);

    std::vector<std::string> expected_error_log2 = {
        "The command `echo h; asdfasdf` has exited with code 32512",
        "=== Stdout:\nh\n",
        "=== Stderr:\nsh: 1: asdfasdf: not found\n"};
    REQUIRE(logger->errors == expected_error_log2);
}
