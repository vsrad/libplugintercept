#include "../src/code_object_substitutor.hpp"
#include "log_helper.hpp"
#include <catch2/catch.hpp>

using namespace agent;

auto _dummy_loader = std::make_shared<CodeObjectLoader>(std::shared_ptr<CoreApiTable>());

TEST_CASE("substitutes code object based on CRC match", "[co_substitutor]")
{
    RecordedCodeObject co_matching("CODE OBJECT", sizeof("CODE OBJECT"), 1, {});
    RecordedCodeObject co_other("SOME OTHER CO", sizeof("SOME OTHER CO"), 2, {});
    REQUIRE(co_matching.crc() != co_other.crc());

    TestLogger logger;
    std::vector<config::CodeObjectSubstitute> subs = {
        {.condition_crc = co_matching.crc(),
         .condition_load_id = {},
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
    std::vector<config::CodeObjectSubstitute> subs =
        {{.condition_crc = {},
          .condition_load_id = 3,
          .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSubstitutor cosw(subs, {}, logger, *_dummy_loader);
    REQUIRE(!cosw.substitute({0}, RecordedCodeObject("", 0, 1, {}), {}));
    auto matching = cosw.substitute({0}, RecordedCodeObject("", 0, 3, {}), {});
    REQUIRE(!cosw.substitute({0}, RecordedCodeObject("", 0, 4, {}), {}));
    REQUIRE(matching);
    std::string subbed_data(static_cast<const char*>(matching->ptr()), matching->size());
    REQUIRE(subbed_data == "ASDF\n");
}

TEST_CASE("substitutes code object based on load call and CRC match", "[co_substitutor]")
{
    RecordedCodeObject co_load1("CO", sizeof("CO"), 1, {});
    RecordedCodeObject co_load2("CO", sizeof("CO"), 2, {});
    RecordedCodeObject co_load3("CO", sizeof("CO"), 3, {});
    REQUIRE(co_load1.crc() == co_load2.crc());

    TestLogger logger;
    std::vector<config::CodeObjectSubstitute> subs = {
        {.condition_crc = co_load1.crc(),
         .condition_load_id = 2,
         .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSubstitutor cosw(subs, {}, logger, *_dummy_loader);
    REQUIRE(!cosw.substitute({0}, co_load1, {}));
    auto matching = cosw.substitute({0}, co_load2, {});
    REQUIRE(!cosw.substitute({0}, co_load3, {}));
    REQUIRE(matching);
    std::string subbed_data(static_cast<const char*>(matching->ptr()), matching->size());
    REQUIRE(subbed_data == "ASDF\n");
}
