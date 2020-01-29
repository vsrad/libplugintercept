#include "code_object_loader.hpp"

using namespace agent;

hsa_status_t CodeObjectLoader::load_from_memory(
    const CodeObject& co,
    hsa_code_object_reader_t* reader,
    const char** error_callsite)
{
    *error_callsite = "hsa_code_object_reader_create_from_memory";
    return _non_intercepted_api_table->hsa_code_object_reader_create_from_memory_fn(co.ptr(), co.size(), reader);
}

hsa_status_t CodeObjectLoader::load_from_memory(
    const CodeObject& co,
    hsa_code_object_t* hsaco,
    const char** error_callsite)
{
    *error_callsite = "hsa_code_object_deserialize";
    return _non_intercepted_api_table->hsa_code_object_deserialize_fn(const_cast<void*>(co.ptr()), co.size(), NULL, hsaco);
}

hsa_status_t CodeObjectLoader::create_executable(
    const CodeObject& co,
    hsa_agent_t agent,
    hsa_executable_t* executable,
    const char** error_callsite)
{
    hsa_code_object_reader_t co_reader;
    hsa_status_t status = load_from_memory(co, &co_reader, error_callsite);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    *error_callsite = "hsa_executable_create";
    status = _non_intercepted_api_table->hsa_executable_create_fn(
        HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, NULL, executable);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    *error_callsite = "hsa_executable_load_agent_code_object";
    status = _non_intercepted_api_table->hsa_executable_load_agent_code_object_fn(
        *executable, agent, co_reader, NULL, NULL);

    return status;
}
