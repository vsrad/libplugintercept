#include "../src/code_object_substitutor.hpp"
#include "log_helper.hpp"
#include <catch2/catch.hpp>
#include <ctime>

using namespace agent;

auto _dummy_loader = std::make_shared<CodeObjectLoader>(std::shared_ptr<CoreApiTable>());

TEST_CASE("substitutes code object based on CRC match", "[co_substitutor]")
{
    RecordedCodeObject co_matching("CODE OBJECT", sizeof("CODE OBJECT"), 1);
    RecordedCodeObject co_other("SOME OTHER CO", sizeof("SOME OTHER CO"), 2);
    REQUIRE(co_matching.crc() != co_other.crc());

    TestLogger logger;
    std::vector<CodeObjectSubstitute> subs = {
        {.condition = {co_matching.crc()},
         .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSubstitutor cosw(subs, {}, logger, *_dummy_loader);
    auto matching = cosw.substitute({0}, co_matching, {});
    auto other = cosw.substitute({0}, co_other, {});
    REQUIRE(matching);
    REQUIRE(!other);
    std::string subbed_data(static_cast<const char*>(matching->ptr()), matching->size());
    REQUIRE(subbed_data == "ASDF\n");
}

TEST_CASE("substitutes code object based on load call match", "[co_substitutor]")
{
    TestLogger logger;
    std::vector<CodeObjectSubstitute> subs =
        {{.condition = {.crc = {}, .load_call_id = 3},
          .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSubstitutor cosw(subs, {}, logger, *_dummy_loader);
    REQUIRE(!cosw.substitute({0}, RecordedCodeObject("", 0, 1), {}));
    auto matching = cosw.substitute({0}, RecordedCodeObject("", 0, 3), {});
    REQUIRE(!cosw.substitute({0}, RecordedCodeObject("", 0, 4), {}));
    REQUIRE(matching);
    std::string subbed_data(static_cast<const char*>(matching->ptr()), matching->size());
    REQUIRE(subbed_data == "ASDF\n");
}

TEST_CASE("substitutes code object based on load call and CRC match", "[co_substitutor]")
{
    RecordedCodeObject co_load1("CO", sizeof("CO"), 1);
    RecordedCodeObject co_load2("CO", sizeof("CO"), 2);
    RecordedCodeObject co_load3("CO", sizeof("CO"), 3);
    REQUIRE(co_load1.crc() == co_load2.crc());

    TestLogger logger;
    std::vector<CodeObjectSubstitute> subs = {
        {.condition = {.crc = co_load1.crc(), .load_call_id = 2},
         .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSubstitutor cosw(subs, {}, logger, *_dummy_loader);
    REQUIRE(!cosw.substitute({0}, co_load1, {}));
    auto matching = cosw.substitute({0}, co_load2, {});
    REQUIRE(!cosw.substitute({0}, co_load3, {}));
    REQUIRE(matching);
    std::string subbed_data(static_cast<const char*>(matching->ptr()), matching->size());
    REQUIRE(subbed_data == "ASDF\n");
}

TEST_CASE("runs external command before substitution if specified", "[co_substitutor]")
{
    auto time = std::time(0);
    TestLogger logger;
    std::vector<CodeObjectSubstitute> subs =
        {{.condition = {.crc = {}, .load_call_id = 1},
          .replacement_path = "tests/tmp/co_substitutor_time",
          .external_command = "echo " + std::to_string(time) + " > tests/tmp/co_substitutor_time"}};
    CodeObjectSubstitutor cosw(subs, {}, logger, *_dummy_loader);
    auto subbed = cosw.substitute({0}, RecordedCodeObject("", 0, 1), {});
    REQUIRE(subbed);
    std::string subbed_data(static_cast<const char*>(subbed->ptr()), subbed->size());
    REQUIRE(subbed_data == std::to_string(time) + "\n");
}
