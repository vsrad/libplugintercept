#include "../src/code_object_swapper.hpp"
#include <catch2/catch.hpp>
#include <ctime>

using namespace agent;

struct TestLogger : AgentLogger
{
    TestLogger() : AgentLogger("-") {}
    std::vector<std::string> infos;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    virtual void info(const std::string& msg) override { infos.push_back(msg); }
    virtual void error(const std::string& msg) override { errors.push_back(msg); }
    virtual void warning(const std::string& msg) override { warnings.push_back(msg); }
};

auto _dummy_loader = std::make_shared<CodeObjectLoader>(std::shared_ptr<CoreApiTable>());

TEST_CASE("swaps code object based on CRC match", "[co_swapper]")
{
    RecordedCodeObject co_matching("CODE OBJECT", sizeof("CODE OBJECT"), 1);
    RecordedCodeObject co_other("SOME OTHER CO", sizeof("SOME OTHER CO"), 2);
    REQUIRE(co_matching.crc() != co_other.crc());

    TestLogger logger;
    DebugBuffer buffer;
    std::vector<CodeObjectSwap> swaps = {{.condition = {co_matching.crc()},
                                          .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSwapper cosw(swaps, logger, *_dummy_loader);
    auto swapped_matching = cosw.try_swap(co_matching, buffer, {0});
    auto swapped_other = cosw.try_swap(co_other, buffer, {0});
    REQUIRE(swapped_matching);
    REQUIRE(!swapped_other);
    std::string swapped_data(static_cast<const char*>(
                                 swapped_matching.replacement_co->ptr()),
                             swapped_matching.replacement_co->size());
    REQUIRE(swapped_data == "ASDF\n");
}

TEST_CASE("swaps code object based on the call # match", "[co_swapper]")
{
    TestLogger logger;
    DebugBuffer buffer;
    std::vector<CodeObjectSwap> swaps =
        {{.condition = {call_count_t(3)},
          .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSwapper cosw(swaps, logger, *_dummy_loader);
    REQUIRE(!cosw.try_swap(RecordedCodeObject("", 0, 1), buffer, {0}));
    auto matching = cosw.try_swap(RecordedCodeObject("", 0, 3), buffer, {0});
    REQUIRE(!cosw.try_swap(RecordedCodeObject("", 0, 4), buffer, {0}));
    REQUIRE(matching);
    std::string swapped_data(static_cast<const char*>(matching.replacement_co->ptr()),
                             matching.replacement_co->size());
    REQUIRE(swapped_data == "ASDF\n");
}

TEST_CASE("runs external command before swapping the code object if specified", "[co_swapper]")
{
    auto time = std::time(0);
    TestLogger logger;
    DebugBuffer buffer;
    std::vector<CodeObjectSwap> swaps =
        {{.condition = {call_count_t(1)},
          .replacement_path = "tests/tmp/co_swapper_time",
          .external_command = "echo " + std::to_string(time) + " > tests/tmp/co_swapper_time"}};
    CodeObjectSwapper cosw(swaps, logger, *_dummy_loader);
    auto swapped = cosw.try_swap(RecordedCodeObject("", 0, 1), buffer, {0});
    REQUIRE(swapped);
    std::string swapped_data(static_cast<const char*>(swapped.replacement_co->ptr()),
                             swapped.replacement_co->size());
    REQUIRE(swapped_data == std::to_string(time) + "\n");
}

TEST_CASE("logs external command output on failure", "[co_swapper]")
{
    TestLogger logger;
    DebugBuffer buffer;
    std::vector<CodeObjectSwap> swaps =
        {{.condition = {call_count_t(1)},
          .replacement_path = "tests/fixtures/asdf",
          .external_command = "asdfasdf"},
         {.condition = {call_count_t(2)},
          .replacement_path = "t",
          .external_command = "echo h; asdfasdf"}};
    CodeObjectSwapper cosw(swaps,
                           logger, *_dummy_loader);
    auto swapped = cosw.try_swap(RecordedCodeObject("SOME CO", sizeof("SOME CO"), 1), buffer, {0});
    REQUIRE(!swapped);

    std::vector<std::string> expected_error_log = {
        "The command `asdfasdf` has exited with code 32512"
        "\n=== Stderr:\nsh: 1: asdfasdf: not found\n"};
    REQUIRE(logger.errors == expected_error_log);

    logger.errors.clear();
    auto swapped2 = cosw.try_swap(RecordedCodeObject("SOME CO", sizeof("SOME CO"), 2), buffer, {0});
    REQUIRE(!swapped2);

    std::vector<std::string> expected_error_log2 = {
        "The command `echo h; asdfasdf` has exited with code 32512"
        "\n=== Stdout:\nh\n"
        "\n=== Stderr:\nsh: 1: asdfasdf: not found\n"};
    REQUIRE(logger.errors == expected_error_log2);
}
