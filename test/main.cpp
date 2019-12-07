#include <fstream>
#include <unistd.h>
#include "kernel_runner.hpp"

int main(void)
{
    const char* tools = getenv("HSA_TOOLS_LIB");
    std::cout << tools << std::endl;

    setenv("ASM_DBG_BUF_SIZE", "4194304", 1);

    KernelRunner runner;
    runner.load_code_object("build/test/kernels/dbg_kernel.co", "dbg_kernel");
    runner.init_dispatch_packet(1, 1);
    runner.dispatch_kernel();
    runner.await_kernel_completion();

    return 0;
}
