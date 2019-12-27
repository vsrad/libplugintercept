#include "../src/CodeObjectManager.hpp"
#include <catch2/catch.hpp>
#include <ctime>

#define DUMP_PATH "tests/tmp"

using namespace agent;

struct TestCodeObjectLogger : CodeObjectLoggerInterface
{
    TestCodeObjectLogger() {}
    std::vector<std::string> infos;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    virtual void info(const std::string& msg) { infos.push_back(msg); }
    virtual void error(const std::string& msg) { errors.push_back(msg); }
    virtual void warning(const std::string& msg) { warnings.push_back(msg); }

    virtual void info(const CodeObject& co, const std::string& msg) { infos.push_back(std::to_string(co.CRC()) + " " + msg); }
    virtual void error(const CodeObject& co, const std::string& msg) { errors.push_back(std::to_string(co.CRC()) + " " + msg); }
    virtual void warning(const CodeObject& co, const std::string& msg) { warnings.push_back(std::to_string(co.CRC()) + " " + msg); }
};

TEST_CASE("init different code objects", "[co_manager]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectManager manager(DUMP_PATH, logger);

    hsa_code_object_t co = {};
    auto co_one = manager.InitCodeObject("CODE OBJECT ONE", sizeof("CODE OBJECT ONE"), &co);
    hsa_code_object_reader_t co_reader = {};
    auto co_two = manager.InitCodeObject("CODE OBJECT TWO", sizeof("CODE OBJECT TWO"), &co_reader);

    REQUIRE(co_one->CRC() != co_two->CRC());
    std::vector<std::string> expected_info = {
        "2005276243 intercepted code object",
        "428259000 intercepted code object"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->warnings.size() == 0);
    REQUIRE(logger->errors.size() == 0);
}

TEST_CASE("redundant load code objects", "[co_manager]")
{
    const char* CODE_OBJECT_DATA = "CODE OBJECT";

    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectManager manager(DUMP_PATH, logger);

    hsa_code_object_t co = {};
    auto co_one = manager.InitCodeObject(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA), &co);
    manager.WriteCodeObject(co_one);

    hsa_code_object_reader_t co_reader = {};
    auto co_two = manager.InitCodeObject(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA), &co_reader);

    REQUIRE(co_one->CRC() == co_two->CRC());
    std::vector<std::string> expected_info = {
        "4212875390 intercepted code object",
        "4212875390 code object is written to the file tests/tmp/4212875390.co",
        "4212875390 intercepted code object"};
    std::vector<std::string> expected_warning = {
        "4212875390 redundant load: tests/tmp/4212875390.co"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->warnings == expected_warning);
    REQUIRE(logger->errors.size() == 0);
}

TEST_CASE("iterate symbols not existed code object", "[co_manager]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectManager manager(DUMP_PATH, logger);

    hsa_code_object_reader_t co_reader = {123};
    hsa_code_object_t co = {456};
    hsa_executable_t exec = {0};
    manager.iterate_symbols(exec, co_reader);
    manager.iterate_symbols(exec, co);

    std::vector<std::string> expected_error = {
        "cannot find code object by hsa_code_object_reader_t: 123",
        "cannot find code object by hsa_code_object_t: 456"};
    REQUIRE(logger->errors == expected_error);
}

TEST_CASE("iterate symbols existed code object with invalid executable", "[co_manager]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectManager manager(DUMP_PATH, logger);

    hsa_code_object_reader_t co_reader = {123};
    hsa_code_object_t co = {456};
    hsa_executable_t exec = {789};
    auto co_one = manager.InitCodeObject("CODE OBJECT ONE", sizeof("CODE OBJECT ONE"), &co);
    auto co_two = manager.InitCodeObject("CODE OBJECT TWO", sizeof("CODE OBJECT TWO"), &co_reader);

    manager.iterate_symbols(exec, co);
    manager.iterate_symbols(exec, co_reader);

    std::vector<std::string> expected_info = {
        "2005276243 intercepted code object",
        "428259000 intercepted code object"};
    std::vector<std::string> expected_error = {
        "428259000 cannot iterate symbols of executable: 789"
        "2005276243 cannot iterate symbols of executable: 789"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->errors == expected_error);
}