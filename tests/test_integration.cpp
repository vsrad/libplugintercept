#include "kernel_runner.hpp"
#include <catch2/catch.hpp>

TEST_CASE("kernel runs to completion", "[integration]")
{
    {
        KernelRunner runner;
        runner.load_code_object("build/tests/kernels/dbg_kernel.co", "dbg_kernel");
        runner.init_dispatch_packet(1, 1);
        runner.dispatch_kernel();
        runner.await_kernel_completion();
    }

    auto co_dump_log = std::ifstream("tests/tmp/co_dump.log");
    REQUIRE(co_dump_log);

    std::string line;
    REQUIRE(std::getline(co_dump_log, line));
    REQUIRE(line == "[CO INFO] crc: 326705597 intercepted code object");
    REQUIRE(std::getline(co_dump_log, line));
    REQUIRE(line == "[CO INFO] crc: 326705597 code object is written to tests/tmp//326705597.co");
    REQUIRE(std::getline(co_dump_log, line));
    REQUIRE(line == "[CO INFO] crc: 326705597 code object symbols: dbg_kernel");

    auto debug_buffer = std::ifstream("tests/tmp/debug_buffer", std::ios::binary | std::ios::ate);
    REQUIRE(debug_buffer);

    size_t filesize = debug_buffer.tellg();
    std::vector<uint32_t> dwords(filesize / 4);
    debug_buffer.seekg(0, std::ios::beg);
    debug_buffer.read(reinterpret_cast<char*>(dwords.data()), filesize);

    std::vector<uint32_t> v_tid_values;
    for (uint32_t dword_idx = 1 /* skip system */; dword_idx < dwords.size(); dword_idx += 2 /* skip system */)
        v_tid_values.push_back(dwords[dword_idx]);
    REQUIRE(!v_tid_values.empty());
    REQUIRE(v_tid_values[0] == 0);
}
