#include "../src/code_object.hpp"
#include "kernel_runner.hpp"
#include <catch2/catch.hpp>

std::vector<uint32_t> load_debug_buffer()
{
    auto debug_buffer = std::ifstream("tests/tmp/debug_buffer", std::ios::binary | std::ios::ate);
    REQUIRE(debug_buffer);

    size_t filesize = debug_buffer.tellg();
    std::vector<uint32_t> dwords(filesize / 4);
    debug_buffer.seekg(0, std::ios::beg);
    debug_buffer.read(reinterpret_cast<char*>(dwords.data()), filesize);

    return dwords;
}

// TEST_CASE("kernel runs to completion", "[integration]")
// {
//     system("rm -r tests/tmp; mkdir tests/tmp");
//     {
//         KernelRunner runner;
//         runner.load_code_object("build/tests/kernels/dbg_kernel.co", "dbg_kernel");
//         runner.init_dispatch_packet(64, 64);
//         runner.dispatch_kernel();
//         runner.await_kernel_completion();
//     }

//     agent::CodeObject co{*agent::CodeObject::try_read_from_file("build/tests/kernels/dbg_kernel.co")};
//     auto crc = std::to_string(co.crc());

//     auto co_dump_log = std::ifstream("tests/tmp/co_dump.log");
//     REQUIRE(co_dump_log);

//     std::string line;
//     REQUIRE(std::getline(co_dump_log, line));
//     REQUIRE(line == "[CO INFO] crc: " + crc + " intercepted code object");
//     REQUIRE(std::getline(co_dump_log, line));
//     REQUIRE(line == "[CO INFO] crc: " + crc + " code object is written to tests/tmp//" + crc + ".co");
//     REQUIRE(std::getline(co_dump_log, line));
//     REQUIRE(line == "[CO INFO] crc: " + crc + " code object symbols: dbg_kernel");

//     auto dwords = load_debug_buffer();
//     REQUIRE(dwords[0] == 0x7777777);
//     uint32_t tid = 0;
//     for (uint32_t dword_idx = 1 /* skip system */; dword_idx < dwords.size() && tid < 64; dword_idx += 2 /* skip system */)
//         REQUIRE(dwords[dword_idx] == tid++);
// }

TEST_CASE("trap handler is properly set up", "[integration]")
{
    system("rm -r tests/tmp; mkdir tests/tmp; cp build/tests/kernels/trap_handler.co tests/tmp");
    {
        KernelRunner runner;
        runner.load_code_object("build/tests/kernels/dbg_kernel.co", "dbg_kernel");
        runner.init_dispatch_packet(64, 64);
        runner.dispatch_kernel();
        runner.await_kernel_completion();
    }

    auto dwords = load_debug_buffer();
    REQUIRE(dwords[0] == 0x7777777);
    uint32_t tid = 0;
    for (uint32_t dword_idx = 1 /* skip system */; dword_idx < dwords.size() && tid < 64; dword_idx += 2 /* skip system */)
        REQUIRE(dwords[dword_idx] == tid++);
}
