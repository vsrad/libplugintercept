#include "config.hpp"
#include "DebugAgent.hpp"
#include <iostream>

std::unique_ptr<agent::DebugAgent> _debug_agent;
std::unique_ptr<CoreApiTable> _intercepted_api_table;

hsa_status_t intercept_hsa_queue_create(
    hsa_agent_t agent,
    uint32_t size,
    hsa_queue_type32_t type,
    void (*callback)(hsa_status_t status, hsa_queue_t* source, void* data),
    void* data,
    uint32_t private_segment_size,
    uint32_t group_segment_size,
    hsa_queue_t** queue)
{
    return _debug_agent->intercept_hsa_queue_create(
        _intercepted_api_table->hsa_queue_create_fn,
        agent, size, type, callback, data, private_segment_size, group_segment_size, queue);
}

hsa_status_t intercept_hsa_code_object_reader_create_from_memory(
    const void* code_object,
    size_t size,
    hsa_code_object_reader_t* code_object_reader)
{
    return _debug_agent->intercept_hsa_code_object_reader_create_from_memory(
        _intercepted_api_table->hsa_code_object_reader_create_from_memory_fn,
        code_object, size, code_object_reader);
}

extern "C" bool OnLoad(void* api_table_ptr, uint64_t rt_version, uint64_t failed_tool_cnt, const char* const* failed_tool_names)
{
    try
    {
        auto config = std::make_shared<agent::Config>();
        _debug_agent = std::make_unique<agent::DebugAgent>(config);

        auto api_table = reinterpret_cast<HsaApiTable*>(api_table_ptr);
        _intercepted_api_table = std::make_unique<CoreApiTable>();
        memcpy(_intercepted_api_table.get(), static_cast<const void*>(api_table->core_), sizeof(CoreApiTable));

        api_table->core_->hsa_queue_create_fn = intercept_hsa_queue_create;
        api_table->core_->hsa_code_object_reader_create_from_memory_fn = intercept_hsa_code_object_reader_create_from_memory;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }

    return true;
}

extern "C" void OnUnload()
{
    _debug_agent->write_debug_buffer_to_file();
    _debug_agent.reset();
    _intercepted_api_table.reset();
}
