#pragma once

#define AMD_INTERNAL_BUILD
#include <hsa_api_trace.h>

#include "CodeObject.hpp"
#include <memory>

namespace agent
{
class CodeObjectLoader
{
private:
    std::shared_ptr<CoreApiTable> _non_intercepted_api_table;

public:
    CodeObjectLoader(std::shared_ptr<CoreApiTable> non_intercepted_api_table)
        : _non_intercepted_api_table(non_intercepted_api_table) {}

    hsa_status_t create_executable(
        const CodeObject& co,
        hsa_agent_t agent,
        hsa_executable_t* executable,
        const char** error_callsite);
};
} // namespace agent
