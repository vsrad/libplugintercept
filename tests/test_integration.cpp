#include <catch2/catch.hpp>
#include "kernel_runner.hpp"

TEST_CASE("kernel runs to completion", "[integration]")
{
    setenv("ASM_DBG_BUF_SIZE", "4194304", 1);

    KernelRunner runner;
    runner.load_code_object("build/tests/kernels/dbg_kernel.co", "dbg_kernel");
    runner.init_dispatch_packet(1, 1);
    runner.dispatch_kernel();
    runner.await_kernel_completion();
}
