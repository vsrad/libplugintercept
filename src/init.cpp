#include "init.hpp"
#include <iostream>

std::unique_ptr<agent::DebugAgent> _debug_agent;

hsa_status_t find_region_callback(
    hsa_region_t region,
    void* data)
{
    hsa_region_segment_t segment_id;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment_id);

    if (segment_id == HSA_REGION_SEGMENT_GLOBAL)
    {
        hsa_region_global_flag_t flags;
        hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);

        if (flags & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED)
        {
            _debug_agent->SetSystemRegion(region);
        }

        if (flags & HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED)
        {
            bool host_accessible_region = false;
            hsa_region_get_info(region, (hsa_region_info_t)HSA_AMD_REGION_INFO_HOST_ACCESSIBLE, &host_accessible_region);

            if (!host_accessible_region)
                _debug_agent->SetGpuRegion(region);
        }
    }

    return HSA_STATUS_SUCCESS;
}

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
    return _debug_agent->intercept_hsa_queue_create(agent,
                                                    size,
                                                    type,
                                                    callback,
                                                    data,
                                                    private_segment_size,
                                                    group_segment_size,
                                                    queue);
}

hsa_status_t intercept_hsa_code_object_reader_create_from_memory(
    const void* code_object,
    size_t size,
    hsa_code_object_reader_t* code_object_reader)
{
    return _debug_agent->intercept_hsa_code_object_reader_create_from_memory(code_object,
                                                                             size,
                                                                             code_object_reader);
}

extern "C" bool OnLoad(void* api_table_ptr, uint64_t rt_version, uint64_t failed_tool_cnt, const char* const* failed_tool_names)
{
    try
    {
        auto api_table = reinterpret_cast<HsaApiTable*>(api_table_ptr);
        _debug_agent = std::make_unique<agent::DebugAgent>(*api_table);

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
    _debug_agent->DebugBufferToFile();
    _debug_agent.reset();
}
