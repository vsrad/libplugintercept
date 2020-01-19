#include "code_object_loader.hpp"

using namespace agent;

hsa_status_t CodeObjectLoader::create_executable(
    const CodeObject& co,
    hsa_agent_t agent,
    hsa_executable_t* executable,
    const char** error_callsite)
{
    hsa_code_object_reader_t co_reader;
    hsa_status_t status = _non_intercepted_api_table->hsa_code_object_reader_create_from_memory_fn(
        co.Ptr(), co.Size(), &co_reader);
    if (status != HSA_STATUS_SUCCESS)
    {
        *error_callsite = "hsa_code_object_reader_create_from_memory";
        return status;
    }
    status = _non_intercepted_api_table->hsa_executable_create_fn(
        HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, NULL, executable);
    if (status != HSA_STATUS_SUCCESS)
    {
        *error_callsite = "hsa_executable_create";
        return status;
    }
    status = _non_intercepted_api_table->hsa_executable_load_agent_code_object_fn(
        *executable, agent, co_reader, NULL, NULL);
    if (status != HSA_STATUS_SUCCESS)
        *error_callsite = "hsa_executable_load_agent_code_object";
    return status;
}
