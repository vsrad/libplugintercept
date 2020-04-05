#include "code_object_loader.hpp"

using namespace agent;

hsa_status_t CodeObjectLoader::load_from_memory(hsaco_t* hsaco, const CodeObject& co, const char** error_callsite)
{
    if (auto reader = std::get_if<hsa_code_object_reader_t>(hsaco))
    {
        *error_callsite = "hsa_code_object_reader_create_from_memory";
        return _non_intercepted_api_table->hsa_code_object_reader_create_from_memory_fn(co.ptr(), co.size(), reader);
    }
    if (auto cobj = std::get_if<hsa_code_object_t>(hsaco))
    {
        *error_callsite = "hsa_code_object_deserialize";
        return _non_intercepted_api_table->hsa_code_object_deserialize_fn(const_cast<void*>(co.ptr()), co.size(), NULL, cobj);
    }
    return HSA_STATUS_ERROR;
}

hsa_status_t CodeObjectLoader::create_executable(
    const CodeObject& co,
    hsa_agent_t agent,
    hsa_executable_t* executable,
    const char** error_callsite)
{
    hsa_code_object_reader_t reader;
    *error_callsite = "hsa_code_object_reader_create_from_memory";
    hsa_status_t status = _non_intercepted_api_table->hsa_code_object_reader_create_from_memory_fn(
        co.ptr(), co.size(), &reader);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    *error_callsite = "hsa_executable_create";
    status = _non_intercepted_api_table->hsa_executable_create_fn(
        HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, NULL, executable);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    *error_callsite = "hsa_executable_load_agent_code_object";
    status = _non_intercepted_api_table->hsa_executable_load_agent_code_object_fn(
        *executable, agent, reader, NULL, NULL);

    return status;
}

hsa_status_t CodeObjectLoader::create_symbol_handle(
    hsa_agent_t agent,
    hsa_executable_t executable,
    const char* symbol_name,
    uint64_t* handle,
    const char** error_callsite)
{
    hsa_status_t status = _non_intercepted_api_table->hsa_executable_freeze_fn(executable, NULL);
    if (status != HSA_STATUS_SUCCESS && status != HSA_STATUS_ERROR_FROZEN_EXECUTABLE /* already frozen */)
    {
        *error_callsite = "hsa_executable_freeze";
        return status;
    }
    hsa_executable_symbol_t symbol;
    status = _non_intercepted_api_table->hsa_executable_get_symbol_by_name_fn(
        executable, symbol_name, &agent, &symbol);
    if (status != HSA_STATUS_SUCCESS)
    {
        *error_callsite = "hsa_executable_get_symbol_by_name";
        return status;
    }
    status = _non_intercepted_api_table->hsa_executable_symbol_get_info_fn(
        symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, handle);
    if (status != HSA_STATUS_SUCCESS)
        *error_callsite = "hsa_executable_symbol_get_info";
    return status;
}
