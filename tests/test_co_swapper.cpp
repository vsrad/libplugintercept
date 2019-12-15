#include "../src/CodeObjectSwapper.hpp"
#include <catch2/catch.hpp>

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

    std::vector<CodeObjectSwapRequest> requests = {
        {.condition = {co_matching.CRC()},
         .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSwapper cosw(requests, std::make_shared<TestLogger>());

    auto swapped_matching = cosw.get_swapped_code_object(co_matching);
    auto swapped_other = cosw.get_swapped_code_object(co_other);
    REQUIRE(swapped_matching);
    REQUIRE(!swapped_other);
    std::vector<char> swapped_data(static_cast<const char*>(swapped_matching->Ptr()),
                                   static_cast<const char*>(swapped_matching->Ptr()) + 5);
    std::vector<char> expected_data = {'A', 'S', 'D', 'F', '\n'};
    REQUIRE(swapped_data == expected_data);
}

TEST_CASE("swaps code object based on the call # match", "[co_swapper]")
{
    CodeObject co("SOME CO", sizeof("SOME CO"));
    std::vector<CodeObjectSwapRequest> requests = {
        {.condition = {call_count_t(3)},
         .replacement_path = "tests/fixtures/asdf"}};
    CodeObjectSwapper cosw(requests, std::make_shared<TestLogger>());
    REQUIRE(!cosw.get_swapped_code_object(co));
    REQUIRE(!cosw.get_swapped_code_object(co));
    auto third_call = cosw.get_swapped_code_object(co);
    REQUIRE(third_call);
    std::vector<char> swapped_data(static_cast<const char*>(third_call->Ptr()),
                                   static_cast<const char*>(third_call->Ptr()) + 5);
    std::vector<char> expected_data = {'A', 'S', 'D', 'F', '\n'};
    REQUIRE(swapped_data == expected_data);
}
