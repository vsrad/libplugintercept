#include "../src/code_object.hpp"
#include "kernel_runner.hpp"
#include <catch2/catch.hpp>
#include <dirent.h>
#include <iomanip>
#include <sstream>

std::vector<std::string> list_files(const char* dirpath)
{
    std::vector<std::string> files;
    if (auto dir = opendir(dirpath))
    {
        while (auto entry = readdir(dir))
            if (entry->d_name[0] != '.')
                files.push_back(entry->d_name);
        closedir(dir);
    }
    return files;
}

std::vector<uint32_t> load_debug_buffer()
{
    auto debug_buffer = std::ifstream("tests/tmp/buffers/debug_buffer", std::ios::binary | std::ios::ate);
    REQUIRE(debug_buffer);

    size_t filesize = debug_buffer.tellg();
    std::vector<uint32_t> dwords(filesize / 4);
    debug_buffer.seekg(0, std::ios::beg);
    debug_buffer.read(reinterpret_cast<char*>(dwords.data()), filesize);

    return dwords;
}

TEST_CASE("using trap handler based debug plug with code-object-replace", "[integration]")
{
    system("rm -r tests/tmp");
    {
        KernelRunner runner;
        runner.create_queue();
        runner.load_code_object("build/tests/kernels/dbg_kernel.co", "dbg_kernel");
        runner.init_dispatch_packet(64, 128);
        runner.dispatch_kernel();
        runner.await_kernel_completion();
    }

    auto co = agent::CodeObject::try_read_from_file("build/tests/kernels/dbg_kernel.co");
    std::stringstream co_crc_ss;
    co_crc_ss << std::setfill('0') << std::setw(sizeof(crc32_t) * 2) << std::hex << co->crc();
    auto co_crc = co_crc_ss.str();

    auto co_dump_log = std::ifstream("tests/tmp/co_dump.log");
    REQUIRE(co_dump_log);

    using Catch::Matchers::StartsWith;

    std::string line;
    REQUIRE(std::getline(co_dump_log, line));
    REQUIRE_THAT(line, StartsWith("[CO INFO] CO 0x" + co_crc + " (co-load-id 1): hsa_code_object_reader_create_from_memory("));
    REQUIRE(std::getline(co_dump_log, line));
    REQUIRE(line == "[CO INFO] CO 0x" + co_crc + " (co-load-id 1): written to tests/tmp/code_objects/" + co_crc + ".co");
    REQUIRE(std::getline(co_dump_log, line));
    REQUIRE(line == "[CO INFO] CO 0x" + co_crc + " (co-load-id 1): code object symbols: dbg_kernel");
    REQUIRE(std::getline(co_dump_log, line));
    REQUIRE_THAT(line, StartsWith("[CO INFO] kernel-get-id 1: hsa_executable_symbol_get_info"));
    REQUIRE(std::getline(co_dump_log, line));
    REQUIRE_THAT(line, StartsWith("[CO INFO] kernel-get-id 2: hsa_executable_symbol_get_info"));

    auto dwords = load_debug_buffer();
    REQUIRE(dwords[0] == 0x7777777);
    uint32_t tid = 2;       // counter = 2, the first two iterations will increment tid_dump by one
    uint32_t dword_idx = 1; // skip system
    for (; dword_idx < dwords.size() && tid <= 64; dword_idx += 2 /* skip system */)
    {
        REQUIRE(dwords[dword_idx] == tid);          /* tid from gid = 0 */
        REQUIRE(dwords[1024 + dword_idx] == tid++); /* tid from gid = 1 */
    }

    // check hidden buffer is dumped
    dword_idx = dwords.size() - 1024; // go to gidden buffer adress
    for (auto val = 1; dword_idx < dwords.size() && tid <= 64; dword_idx += 1, val++)
        REQUIRE(dwords[dword_idx] == val * 2);
}

TEST_CASE("using in kernel plug with kernel-replace, init command runs after hsa_queue_create", "[integration]")
{
    system("rm -r tests/tmp; mkdir tests/tmp");
    auto old_env = getenv("INTERCEPT_CONFIG");
    setenv("INTERCEPT_CONFIG", "tests/fixtures/plug.toml", 1);
    {
        KernelRunner runner;

        REQUIRE(0 != system("test -f tests/tmp/init_command_executed")); // init command not executed yet

        runner.create_queue();

        REQUIRE(0 == system("test -f tests/tmp/init_command_executed && test 1 -eq $(stat -c '%s' tests/tmp/init_command_executed)")); // executed after queue_create

        runner.load_code_object("build/tests/kernels/dbg_kernel.co", "dbg_kernel");
        runner.init_dispatch_packet(64, 64);
        runner.dispatch_kernel();
        runner.await_kernel_completion();

        REQUIRE(0 == system("test -f tests/tmp/init_command_executed && test 1 -eq $(stat -c '%s' tests/tmp/init_command_executed)")); // executed only once
    }
    setenv("INTERCEPT_CONFIG", old_env, 1);

    // no logs created
    auto tests_tmp = list_files("tests/tmp");
    std::vector<std::string> expected_tests_tmp = {"buffers", "init_command_executed", "replacement_plug.co"};
    REQUIRE(tests_tmp == expected_tests_tmp);

    auto dwords = load_debug_buffer();
    REQUIRE(dwords[0] == 0x7777777);
    uint32_t tid = 0;
    for (uint32_t dword_idx = 1 /* skip system */; dword_idx < dwords.size() && tid < 64; dword_idx += 2 /* skip system */)
        REQUIRE(dwords[dword_idx] == tid++);
}

TEST_CASE("using in kernel plug with kernel-replace, init command run after hsa_executable_load_agent_code_object", "[integration]")
{
    system("rm -r tests/tmp; mkdir tests/tmp");
    auto old_env = getenv("INTERCEPT_CONFIG");
    setenv("INTERCEPT_CONFIG", "tests/fixtures/plug.toml", 1);
    {
        KernelRunner runner;

        REQUIRE(0 != system("test -f tests/tmp/init_command_executed")); // init command not executed yet

        runner.load_code_object("build/tests/kernels/dbg_kernel.co", "dbg_kernel");

        REQUIRE(0 == system("test -f tests/tmp/init_command_executed && test 1 -eq $(stat -c '%s' tests/tmp/init_command_executed)")); // executed after load_code_object

        runner.create_queue();
        runner.init_dispatch_packet(64, 64);
        runner.dispatch_kernel();
        runner.await_kernel_completion();

        REQUIRE(0 == system("test -f tests/tmp/init_command_executed && test 1 -eq $(stat -c '%s' tests/tmp/init_command_executed)")); // executed only onc
    }
    setenv("INTERCEPT_CONFIG", old_env, 1);
}

TEST_CASE("log-only mode", "[integration]")
{
    system("rm -r tests/tmp");
    auto old_env = getenv("INTERCEPT_CONFIG");
    setenv("INTERCEPT_CONFIG", "tests/fixtures/minimal.toml", 1);
    {
        KernelRunner runner;
        runner.create_queue();
        runner.load_code_object("build/tests/kernels/dbg_kernel.co", "dbg_kernel");
        runner.load_code_object("build/tests/kernels/dbg_kernel.co", "dbg_kernel"); // trigger redundant load warning
        runner.init_dispatch_packet(64, 64);
        runner.dispatch_kernel();
        runner.await_kernel_completion();
    }
    setenv("INTERCEPT_CONFIG", old_env, 1);
    auto tests_tmp = list_files("tests/tmp");
    std::vector<std::string> expected_tests_tmp = {"logs"};
    REQUIRE(tests_tmp == expected_tests_tmp);
    auto tests_tmp_logs = list_files("tests/tmp/logs");
    std::vector<std::string> expected_tests_tmp_logs = {"agent.log", "co.log"};
    REQUIRE(tests_tmp_logs == expected_tests_tmp_logs);
}
