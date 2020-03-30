#include "../src/code_object_recorder.hpp"
#include <catch2/catch.hpp>
#include <ctime>

#define DUMP_PATH "tests/tmp"

using namespace agent;

struct TestCodeObjectLogger : CodeObjectLogger
{
    TestCodeObjectLogger() : CodeObjectLogger("-") {}
    std::vector<std::string> infos;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    virtual void info(const std::string& msg) override { infos.push_back(msg); }
    virtual void error(const std::string& msg) override { errors.push_back(msg); }
    virtual void warning(const std::string& msg) override { warnings.push_back(msg); }
};

TEST_CASE("init different code objects", "[co_recorder]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectRecorder recorder(DUMP_PATH, logger);

    auto& co_one = recorder.record_code_object("CODE OBJECT ONE", sizeof("CODE OBJECT ONE"));
    auto& co_two = recorder.record_code_object("CODE OBJECT TWO", sizeof("CODE OBJECT TWO"));
    auto& co_three = recorder.record_code_object("CODE OBJECT THREE", sizeof("CODE OBJECT THREE"));
    REQUIRE(co_one.load_call_id() == 1);
    REQUIRE(co_two.load_call_id() == 2);
    REQUIRE(co_three.load_call_id() == 3);

    REQUIRE(co_one.crc() != co_two.crc());
    REQUIRE(co_one.crc() != co_three.crc());
    std::vector<std::string> expected_info = {
        "crc: 2005276243 intercepted code object",
        "crc: 2005276243 code object is written to tests/tmp/2005276243.co",
        "crc: 428259000 intercepted code object",
        "crc: 428259000 code object is written to tests/tmp/428259000.co",
        "crc: 4099621386 intercepted code object",
        "crc: 4099621386 code object is written to tests/tmp/4099621386.co"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->warnings.size() == 0);
    REQUIRE(logger->errors.size() == 0);
}

TEST_CASE("dump code object to an invalid path", "[co_recorder]")
{
    const char* CODE_OBJECT_DATA = "CODE OBJECT";

    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectRecorder recorder("invalid-path", logger);
    recorder.record_code_object(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA));

    std::vector<std::string> expected_info = {
        "crc: 4212875390 intercepted code object"};
    std::vector<std::string> expected_error = {
        "crc: 4212875390 cannot write code object to invalid-path/4212875390.co"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->errors == expected_error);
    REQUIRE(logger->warnings.size() == 0);
}

TEST_CASE("redundant load code objects", "[co_recorder]")
{
    const char* CODE_OBJECT_DATA = "CODE OBJECT";

    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectRecorder recorder(DUMP_PATH, logger);

    auto& co_one = recorder.record_code_object(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA));
    auto& co_two = recorder.record_code_object(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA));

    REQUIRE(co_one.crc() == co_two.crc());
    std::vector<std::string> expected_info = {
        "crc: 4212875390 intercepted code object",
        "crc: 4212875390 code object is written to tests/tmp/4212875390.co",
        "crc: 4212875390 intercepted code object"};
    std::vector<std::string> expected_warning = {
        "crc: 4212875390 redundant load: tests/tmp/4212875390.co"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->warnings == expected_warning);
    REQUIRE(logger->errors.size() == 0);
}

TEST_CASE("find code object called on a nonexistent code object", "[co_recorder]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectRecorder recorder(DUMP_PATH, logger);

    hsa_code_object_reader_t co_reader = {123};
    hsa_code_object_t co = {456};
    REQUIRE(!recorder.find_code_object(co_reader));
    REQUIRE(!recorder.find_code_object(co));

    std::vector<std::string> expected_error = {
        "cannot find code object by hsa_code_object_reader_t: 123",
        "cannot find code object by hsa_code_object_t: 456"};
    REQUIRE(logger->errors == expected_error);
    REQUIRE(logger->infos.size() == 0);
    REQUIRE(logger->warnings.size() == 0);
}

TEST_CASE("find code object called with an invalid executable", "[co_recorder]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectRecorder recorder(DUMP_PATH, logger);

    // Create hsa_code_object_t and hsa_code_object_reader_t in separate functions
    // to verify that CodeObjectRecorder does not reply on pointers to them, which may be
    // unreliable.
    auto prepare_co_one = [&]() -> std::tuple<hsa_code_object_t, const RecordedCodeObject&> {
        hsa_code_object_t co = {12};
        auto& co_one = recorder.record_code_object("CODE OBJECT ONE", sizeof("CODE OBJECT ONE"));
        co_one.set_hsa_code_object(co);
        return {co, co_one};
    };
    auto prepare_co_two = [&]() -> std::tuple<hsa_code_object_reader_t, const RecordedCodeObject&> {
        hsa_code_object_reader_t co_reader = {23};
        auto& co_two = recorder.record_code_object("CODE OBJECT TWO", sizeof("CODE OBJECT TWO"));
        co_two.set_hsa_code_object_reader(co_reader);
        return {co_reader, co_two};
    };

    auto [co, co_one] = prepare_co_one();
    auto [co_reader, co_two] = prepare_co_two();
    REQUIRE(co_one.crc() != co_two.crc());

    auto recorded_one = recorder.find_code_object(co);
    auto recorded_two = recorder.find_code_object(co_reader);
    REQUIRE(recorded_one);
    REQUIRE(recorded_one->get().load_call_id() == co_one.load_call_id());
    REQUIRE(recorded_one->get().load_call_id() == 1);
    REQUIRE(recorded_two);
    REQUIRE(recorded_two->get().load_call_id() == co_two.load_call_id());
    REQUIRE(recorded_two->get().load_call_id() == 2);

    std::vector<std::string> expected_info = {
        "crc: 2005276243 intercepted code object",
        "crc: 2005276243 code object is written to tests/tmp/2005276243.co",
        "crc: 428259000 intercepted code object",
        "crc: 428259000 code object is written to tests/tmp/428259000.co"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->warnings.size() == 0);
}
