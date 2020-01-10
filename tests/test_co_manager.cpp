#include "../src/CodeObjectManager.hpp"
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

TEST_CASE("init different code objects", "[co_manager]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectManager manager(DUMP_PATH, logger);

    auto co_one = manager.record_code_object("CODE OBJECT ONE", sizeof("CODE OBJECT ONE"));
    auto co_two = manager.record_code_object("CODE OBJECT TWO", sizeof("CODE OBJECT TWO"));

    REQUIRE(co_one->CRC() != co_two->CRC());
    std::vector<std::string> expected_info = {
        "crc: 2005276243 intercepted code object",
        "crc: 2005276243 code object is written to the file tests/tmp/2005276243.co",
        "crc: 428259000 intercepted code object",
        "crc: 428259000 code object is written to the file tests/tmp/428259000.co"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->warnings.size() == 0);
    REQUIRE(logger->errors.size() == 0);
}

TEST_CASE("dump code object to an invalid path", "[co_manager]")
{
    const char* CODE_OBJECT_DATA = "CODE OBJECT";

    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectManager manager("invalid-path", logger);
    auto co_one = manager.record_code_object(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA));

    std::vector<std::string> expected_info = {
        "crc: 4212875390 intercepted code object"};
    std::vector<std::string> expected_error = {
        "crc: 4212875390 cannot write code object to the file invalid-path/4212875390.co"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->errors == expected_error);
    REQUIRE(logger->warnings.size() == 0);
}

TEST_CASE("redundant load code objects", "[co_manager]")
{
    const char* CODE_OBJECT_DATA = "CODE OBJECT";

    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectManager manager(DUMP_PATH, logger);

    auto co_one = manager.record_code_object(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA));
    auto co_two = manager.record_code_object(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA));

    REQUIRE(co_one->CRC() == co_two->CRC());
    std::vector<std::string> expected_info = {
        "crc: 4212875390 intercepted code object",
        "crc: 4212875390 code object is written to the file tests/tmp/4212875390.co",
        "crc: 4212875390 intercepted code object"};
    std::vector<std::string> expected_warning = {
        "crc: 4212875390 redundant load: tests/tmp/4212875390.co"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->warnings == expected_warning);
    REQUIRE(logger->errors.size() == 0);
}

TEST_CASE("iterate symbols called on a nonexistent code object", "[co_manager]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectManager manager(DUMP_PATH, logger);

    hsa_code_object_reader_t co_reader = {123};
    hsa_code_object_t co = {456};
    hsa_executable_t exec = {0};
    manager.set_code_object_executable(exec, co_reader);
    manager.set_code_object_executable(exec, co);

    std::vector<std::string> expected_error = {
        "cannot find code object by hsa_code_object_reader_t: 123",
        "cannot find code object by hsa_code_object_t: 456"};
    REQUIRE(logger->errors == expected_error);
    REQUIRE(logger->infos.size() == 0);
    REQUIRE(logger->warnings.size() == 0);
}

TEST_CASE("iterate symbols called with an invalid executable", "[co_manager]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectManager manager(DUMP_PATH, logger);

    // Create hsa_code_object_t and hsa_code_object_reader_t in separate functions
    // to verify that CodeObjectManager does not reply on pointers to them, which may be
    // unreliable.
    auto prepare_co_one = [&]() -> std::tuple<hsa_code_object_t, std::shared_ptr<CodeObject>> {
        hsa_code_object_t co = {12};
        auto co_one = manager.record_code_object("CODE OBJECT ONE", sizeof("CODE OBJECT ONE"));
        manager.set_code_object_handle(co_one, co);
        return {co, co_one};
    };
    auto prepare_co_two = [&]() -> std::tuple<hsa_code_object_reader_t, std::shared_ptr<CodeObject>> {
        hsa_code_object_reader_t co_reader = {23};
        auto co_two = manager.record_code_object("CODE OBJECT TWO", sizeof("CODE OBJECT TWO"));
        manager.set_code_object_handle(co_two, co_reader);
        return {co_reader, co_two};
    };

    auto [co, co_one] = prepare_co_one();
    auto [co_reader, co_two] = prepare_co_two();
    REQUIRE(co_one->CRC() != co_two->CRC());

    hsa_executable_t exec = {789};
    manager.set_code_object_executable(exec, co);
    manager.set_code_object_executable(exec, co_reader);

    std::vector<std::string> expected_info = {
        "crc: 2005276243 intercepted code object",
        "crc: 2005276243 code object is written to the file tests/tmp/2005276243.co",
        "crc: 428259000 intercepted code object",
        "crc: 428259000 code object is written to the file tests/tmp/428259000.co"};
    std::vector<std::string> expected_error = {
        "crc: 2005276243 cannot iterate symbols of executable: 789",
        "crc: 428259000 cannot iterate symbols of executable: 789"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->errors == expected_error);
    REQUIRE(logger->warnings.size() == 0);
}
