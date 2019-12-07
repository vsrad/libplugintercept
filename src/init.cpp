#define AMD_INTERNAL_BUILD
#include <hsa_api_trace.h>

#include <iostream>

// HSA Runtime function table without intercepts.
CoreApiTable _hsa_core_api_table;

void* _debug_buffer = NULL;

hsa_status_t intercept_hsa_code_object_reader_create_from_memory(
    const void* code_object,
    size_t size,
    hsa_code_object_reader_t* code_object_reader)
{
    const size_t amd_kernel_code_t_offset = 256;
    const uint32_t* instructions = static_cast<const uint32_t*>(code_object + amd_kernel_code_t_offset);
    const uint32_t* instructions_end = static_cast<const uint32_t*>(code_object + size);

    for (; instructions < instructions_end - 2; ++instructions)
    {
        if (*instructions == 0x7f7f7f7f)
        {
            std::cout << "Magic bytes found" << std::endl;
            if (instructions[2] == 0x7f7f7f7f)
            {
                std::cout << "s_mov_b32 found" << std::endl;
                instructions += 2;
            }
        }
    }

    return _hsa_core_api_table.hsa_code_object_reader_create_from_memory_fn(code_object, size, code_object_reader);
}

hsa_status_t find_gpu_region_callback(hsa_region_t region, void* data)
{
    hsa_region_segment_t segment_id;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment_id);

    if (segment_id == HSA_REGION_SEGMENT_GLOBAL)
    {
        hsa_region_global_flag_t flags;
        hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);

        if (flags & HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED)
        {
            bool host_accessible_region = false;
            hsa_region_get_info(region, (hsa_region_info_t)HSA_AMD_REGION_INFO_HOST_ACCESSIBLE, &host_accessible_region);

            if (!host_accessible_region)
                *static_cast<hsa_region_t*>(data) = region;
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
    hsa_status_t status = _hsa_core_api_table.hsa_queue_create_fn(
        agent, size, type, callback, data, private_segment_size, group_segment_size, queue);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    if (const char* buf_size_env = getenv("ASM_DBG_BUF_SIZE"))
    {
        int buf_size = atoi(buf_size_env);

        hsa_region_t gpu_region = {0};
        status = hsa_agent_iterate_regions(agent, find_gpu_region_callback, &gpu_region);
        if (status != HSA_STATUS_SUCCESS || gpu_region.handle == 0)
        {
            std::cerr << "Unable to find GPU region to allocate debug buffer" << std::endl;
            return status;
        }
        status = hsa_memory_allocate(gpu_region, buf_size, &_debug_buffer);
        if (status != HSA_STATUS_SUCCESS)
        {
            std::cerr << "Unable to find GPU region to allocate debug buffer" << std::endl;
            return status;
        }
        std::cout << "Allocated debug buffer of size " << buf_size << " at " << _debug_buffer << std::endl;
    }
    else
    {
        std::cout << "Warning: cannot allocate debug buffer because ASM_DBG_BUF_SIZE is not set" << std::endl;
    }

    return status;
}

extern "C" bool OnLoad(void* api_table_ptr, uint64_t rt_version, uint64_t failed_tool_cnt, const char* const* failed_tool_names)
{
    auto api_table = reinterpret_cast<HsaApiTable*>(api_table_ptr);
    memcpy(static_cast<void*>(&_hsa_core_api_table), static_cast<const void*>(api_table->core_), sizeof(CoreApiTable));

    api_table->core_->hsa_queue_create_fn = intercept_hsa_queue_create;
    api_table->core_->hsa_code_object_reader_create_from_memory_fn = intercept_hsa_code_object_reader_create_from_memory;

    return true;
}
