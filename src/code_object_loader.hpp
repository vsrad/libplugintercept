#pragma once

#define AMD_INTERNAL_BUILD
#include <hsa_api_trace.h>

#include "code_object.hpp"
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

    hsa_status_t load_from_memory(
        const CodeObject& co,
        hsa_code_object_reader_t* reader,
        const char** error_callsite);

    hsa_status_t load_from_memory(
        const CodeObject& co,
        hsa_code_object_t* hsaco,
        const char** error_callsite);

    hsa_status_t create_executable(
        const CodeObject& co,
        hsa_agent_t agent,
        hsa_executable_t* executable,
        const char** error_callsite);

    hsa_status_t create_symbol_handle(
        hsa_agent_t agent,
        hsa_executable_t executable,
        const char* symbol_name,
        uint64_t* handle,
        const char** error_callsite);
};
} // namespace agent
