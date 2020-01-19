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

TEST_CASE("swaps code object based on CRC match", "[co_swapper]")
{
    auto co_matching = std::make_shared<RecordedCodeObject>("CODE OBJECT", sizeof("CODE OBJECT"));
    auto co_other = std::make_shared<RecordedCodeObject>("SOME OTHER CO", sizeof("SOME OTHER CO"));
    REQUIRE(co_matching->crc() != co_other->crc());

    std::vector<CodeObjectSwap> swaps = {{.condition = {co_matching->crc()},
                                          .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSwapper cosw(std::make_shared<std::vector<CodeObjectSwap>>(swaps), std::make_shared<TestLogger>());
    auto swapped_matching = cosw.get_swapped_code_object(co_matching, std::unique_ptr<Buffer>());
    auto swapped_other = cosw.get_swapped_code_object(co_other, std::unique_ptr<Buffer>());
    REQUIRE(swapped_matching);
    REQUIRE(!swapped_other);
    std::string swapped_data(static_cast<const char*>(swapped_matching->ptr()), swapped_matching->size());
    REQUIRE(swapped_data == "ASDF\n");
}

TEST_CASE("swaps code object based on the call # match", "[co_swapper]")
{
    std::vector<CodeObjectSwap> swaps = {{.condition = {call_count_t(3)},
                                          .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSwapper cosw(std::make_shared<std::vector<CodeObjectSwap>>(swaps), std::make_shared<TestLogger>());
    REQUIRE(!cosw.get_swapped_code_object(std::make_shared<RecordedCodeObject>("", 0), std::unique_ptr<Buffer>()));
    REQUIRE(!cosw.get_swapped_code_object(std::make_shared<RecordedCodeObject>("", 0), std::unique_ptr<Buffer>()));
    auto third_call = cosw.get_swapped_code_object(std::make_shared<RecordedCodeObject>("", 0), std::unique_ptr<Buffer>());
    REQUIRE(third_call);
    std::string swapped_data(static_cast<const char*>(third_call->ptr()), third_call->size());
    REQUIRE(swapped_data == "ASDF\n");
}

TEST_CASE("runs external command before swapping the code object if specified", "[co_swapper]")
{
    auto time = std::time(0);
    auto logger = std::make_shared<TestLogger>();
    std::vector<CodeObjectSwap> swaps =
        {{.condition = {call_count_t(1)},
          .replacement_path = "tests/tmp/co_swapper_time",
          .external_command = "echo " + std::to_string(time) + " > tests/tmp/co_swapper_time"}};
    CodeObjectSwapper cosw(std::make_shared<std::vector<CodeObjectSwap>>(swaps), logger);
    auto swapped = cosw.get_swapped_code_object(std::make_shared<RecordedCodeObject>("", 0), std::unique_ptr<Buffer>());
    REQUIRE(swapped);
    std::string swapped_data(static_cast<const char*>(swapped->ptr()), swapped->size());
    REQUIRE(swapped_data == std::to_string(time) + "\n");
}

TEST_CASE("logs external command output on failure", "[co_swapper]")
{
    auto logger = std::make_shared<TestLogger>();
    std::vector<CodeObjectSwap> swaps =
        {{.condition = {call_count_t(1)},
          .replacement_path = "tests/fixtures/asdf",
          .external_command = "asdfasdf"},
         {.condition = {call_count_t(2)},
          .replacement_path = "t",
          .external_command = "echo h; asdfasdf"}};
    CodeObjectSwapper cosw(std::make_shared<std::vector<CodeObjectSwap>>(swaps), logger);
    auto swapped = cosw.get_swapped_code_object(std::make_shared<RecordedCodeObject>("SOME CO", sizeof("SOME CO")), std::unique_ptr<Buffer>());
    REQUIRE(!swapped);

    std::vector<std::string> expected_error_log = {
        "The command `asdfasdf` has exited with code 32512"
        "\n=== Stderr:\nsh: 1: asdfasdf: not found\n"};
    REQUIRE(logger->errors == expected_error_log);

    logger->errors.clear();
    auto swapped2 = cosw.get_swapped_code_object(std::make_shared<RecordedCodeObject>("SOME CO", sizeof("SOME CO")), std::unique_ptr<Buffer>());
    REQUIRE(!swapped2);

    std::vector<std::string> expected_error_log2 = {
        "The command `echo h; asdfasdf` has exited with code 32512"
        "\n=== Stdout:\nh\n"
        "\n=== Stderr:\nsh: 1: asdfasdf: not found\n"};
    REQUIRE(logger->errors == expected_error_log2);
}
