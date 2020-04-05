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

TEST_CASE("keeps record of code objects", "[co_recorder]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectRecorder recorder(DUMP_PATH, logger);

    hsaco_t r1 = hsa_code_object_reader_t{1};
    hsaco_t r2 = hsa_code_object_reader_t{2};
    hsaco_t o1 = hsa_code_object_t{1};
    hsaco_t o2 = hsa_code_object_t{2};

    recorder.record_code_object("CO R1", sizeof("CO R1"), r1, HSA_STATUS_SUCCESS);
    recorder.record_code_object("CO R2", sizeof("CO R2"), r2, HSA_STATUS_SUCCESS);
    recorder.record_code_object("CO ERR", sizeof("CO ERR"), o1, HSA_STATUS_ERROR);
    recorder.record_code_object("CO O2", sizeof("CO O2"), o2, HSA_STATUS_SUCCESS);

    auto cor1 = recorder.find_code_object(&r1);
    REQUIRE(cor1);
    REQUIRE(cor1->get().load_call_id() == 1);
    auto cor2 = recorder.find_code_object(&r2);
    REQUIRE(cor2);
    REQUIRE(cor2->get().load_call_id() == 2);
    auto coo2 = recorder.find_code_object(&o2);
    REQUIRE(coo2);
    REQUIRE(coo2->get().load_call_id() == 4);

    std::vector<std::string> expected_info = {
        "CO 0xebfa44ab (load #1): loaded",
        "CO 0xebfa44ab (load #1): written to tests/tmp/ebfa44ab.co",
        "CO 0xc0d71768 (load #2): loaded",
        "CO 0xc0d71768 (load #2): written to tests/tmp/c0d71768.co",
        "CO 0xd429274b (load #4): loaded",
        "CO 0xd429274b (load #4): written to tests/tmp/d429274b.co"};
    REQUIRE(logger->infos == expected_info);
    std::vector<std::string> expected_warnings = {
        "Load #3 failed with status 4096"
    };
    REQUIRE(logger->warnings == expected_warnings);
    REQUIRE(logger->errors.empty());
}

TEST_CASE("warns when looking up non-existent hsacos", "[co_recorder]")
{
    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectRecorder recorder(DUMP_PATH, logger);

    hsaco_t reader = hsa_code_object_reader_t{123};
    hsaco_t cobj = hsa_code_object_t{456};
    REQUIRE(!recorder.find_code_object(&reader));
    REQUIRE(!recorder.find_code_object(&cobj));

    std::vector<std::string> expected_error = {
        "Cannot find code object by hsa_code_object_reader_t: 123",
        "Cannot find code object by hsa_code_object_t: 456"};
    REQUIRE(logger->errors == expected_error);
    REQUIRE(logger->infos.empty());
    REQUIRE(logger->warnings.empty());
}

TEST_CASE("dump code object to an invalid path", "[co_recorder]")
{
    const char* CODE_OBJECT_DATA = "CODE OBJECT";

    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectRecorder recorder("invalid-path", logger);
    recorder.record_code_object(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA), hsa_code_object_t{1}, HSA_STATUS_SUCCESS);

    std::vector<std::string> expected_info = {
        "CO 0xfb1b607e (load #1): loaded"};
    std::vector<std::string> expected_error = {
        "CO 0xfb1b607e (load #1): cannot write code object to invalid-path/fb1b607e.co"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->errors == expected_error);
    REQUIRE(logger->warnings.empty());
}

TEST_CASE("redundant load code objects", "[co_recorder]")
{
    const char* CODE_OBJECT_DATA = "CODE OBJECT";

    auto logger = std::make_shared<TestCodeObjectLogger>();
    CodeObjectRecorder recorder(DUMP_PATH, logger);

    hsaco_t o1 = hsa_code_object_t{1};
    hsaco_t o2 = hsa_code_object_t{2};

    recorder.record_code_object(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA), o1, HSA_STATUS_SUCCESS);
    recorder.record_code_object(CODE_OBJECT_DATA, sizeof(CODE_OBJECT_DATA), o2, HSA_STATUS_SUCCESS);

    auto coo1 = recorder.find_code_object(&o1);
    auto coo2 = recorder.find_code_object(&o2);
    REQUIRE(coo1->get().crc() == coo2->get().crc());

    std::vector<std::string> expected_info = {
        "CO 0xfb1b607e (load #1): loaded",
        "CO 0xfb1b607e (load #1): written to tests/tmp/fb1b607e.co",
        "CO 0xfb1b607e (load #2): loaded"};
    std::vector<std::string> expected_warning = {
        "CO 0xfb1b607e (load #2): redundant load, same contents as CO 0xfb1b607e (load #1)"};
    REQUIRE(logger->infos == expected_info);
    REQUIRE(logger->warnings == expected_warning);
    REQUIRE(logger->errors.empty());
}
