#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

struct TestSetupListener : Catch::TestEventListenerBase
{
    using TestEventListenerBase::TestEventListenerBase;

    void testRunStarting(Catch::TestRunInfo const& testRunInfo) override
    {
        setenv("INTERCEPT_CONFIG", "tests/fixtures/config.toml", 1);
        setenv("HSA_TOOLS_LIB", "build/src/libplugintercept.so", 1);
        system("rm -r tests/tmp; mkdir tests/tmp");
    }
};

CATCH_REGISTER_LISTENER(TestSetupListener)
