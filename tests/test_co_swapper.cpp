#include "../src/code_object_swapper.hpp"
#include "log_helper.hpp"
#include <catch2/catch.hpp>
#include <ctime>

using namespace agent;

auto _dummy_loader = std::make_shared<CodeObjectLoader>(std::shared_ptr<CoreApiTable>());

TEST_CASE("swaps code object based on CRC match", "[co_swapper]")
{
    RecordedCodeObject co_matching("CODE OBJECT", sizeof("CODE OBJECT"), 1);
    RecordedCodeObject co_other("SOME OTHER CO", sizeof("SOME OTHER CO"), 2);
    REQUIRE(co_matching.crc() != co_other.crc());

    TestLogger logger;
    std::vector<CodeObjectSwap> swaps = {
        {.condition = {co_matching.crc()},
         .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSwapper cosw(swaps, {}, logger, *_dummy_loader);
    auto swapped_matching = cosw.try_swap({0}, co_matching, {});
    auto swapped_other = cosw.try_swap({0}, co_other, {});
    REQUIRE(swapped_matching);
    REQUIRE(!swapped_other);
    std::string swapped_data(
        static_cast<const char*>(swapped_matching.replacement_co->ptr()),
        swapped_matching.replacement_co->size());
    REQUIRE(swapped_data == "ASDF\n");
}

TEST_CASE("swaps code object based on load call match", "[co_swapper]")
{
    TestLogger logger;
    std::vector<CodeObjectSwap> swaps =
        {{.condition = {.crc = {}, .load_call_id = 3},
          .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSwapper cosw(swaps, {}, logger, *_dummy_loader);
    REQUIRE(!cosw.try_swap({0}, RecordedCodeObject("", 0, 1), {}));
    auto matching = cosw.try_swap({0}, RecordedCodeObject("", 0, 3), {});
    REQUIRE(!cosw.try_swap({0}, RecordedCodeObject("", 0, 4), {}));
    REQUIRE(matching);
    std::string swapped_data(
        static_cast<const char*>(matching.replacement_co->ptr()),
        matching.replacement_co->size());
    REQUIRE(swapped_data == "ASDF\n");
}

TEST_CASE("swaps code object based on load call and CRC match", "[co_swapper]")
{
    RecordedCodeObject co_load1("CO", sizeof("CO"), 1);
    RecordedCodeObject co_load2("CO", sizeof("CO"), 2);
    RecordedCodeObject co_load3("CO", sizeof("CO"), 3);
    REQUIRE(co_load1.crc() == co_load2.crc());

    TestLogger logger;
    std::vector<CodeObjectSwap> swaps = {
        {.condition = {.crc = co_load1.crc(), .load_call_id = 2},
         .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSwapper cosw(swaps, {}, logger, *_dummy_loader);
    REQUIRE(!cosw.try_swap({0}, co_load1, {}));
    auto matching = cosw.try_swap({0}, co_load2, {});
    REQUIRE(!cosw.try_swap({0}, co_load3, {}));
    REQUIRE(matching);
    std::string swapped_data(
        static_cast<const char*>(matching.replacement_co->ptr()),
        matching.replacement_co->size());
    REQUIRE(swapped_data == "ASDF\n");
}

TEST_CASE("runs external command before swapping the code object if specified", "[co_swapper]")
{
    auto time = std::time(0);
    TestLogger logger;
    std::vector<CodeObjectSwap> swaps =
        {{.condition = {.crc = {}, .load_call_id = 1},
          .replacement_path = "tests/tmp/co_swapper_time",
          .external_command = "echo " + std::to_string(time) + " > tests/tmp/co_swapper_time"}};
    CodeObjectSwapper cosw(swaps, {}, logger, *_dummy_loader);
    auto swapped = cosw.try_swap({0}, RecordedCodeObject("", 0, 1), {});
    REQUIRE(swapped);
    std::string swapped_data(static_cast<const char*>(swapped->ptr()), swapped->size());
    REQUIRE(swapped_data == std::to_string(time) + "\n");
}
