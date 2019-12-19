#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

struct TestSetupListener : Catch::TestEventListenerBase
{
    using TestEventListenerBase::TestEventListenerBase;

    void testRunStarting(Catch::TestRunInfo const& testRunInfo) override
    {
        setenv("ASM_DBG_CONFIG", "tests/fixtures/config.toml", 1);
        setenv("HSA_TOOLS_LIB", "build/src/libplugintercept.so", 1);
    }
};

CATCH_REGISTER_LISTENER(TestSetupListener)
