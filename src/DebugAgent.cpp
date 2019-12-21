#include "DebugAgent.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace agent;

void DebugAgent::write_debug_buffer_to_file()
{
    if (!_debug_buffer)
    {
        _logger->info("Debug buffer is not allocated");
        return;
    }

    hsa_status_t status;
    status = hsa_memory_copy(_debug_buffer->SystemPtr(), _debug_buffer->LocalPtr(), _debug_buffer->Size());
    if (status != HSA_STATUS_SUCCESS)
    {
        _logger->error("Unable to copy GPU local memory to system memory for debug buffer");
        return;
    }

    std::ofstream fs(_config->debug_buffer_dump_file(), std::ios::out | std::ios::binary);
    if (!fs.is_open())
    {
        _logger->error("Failed to open " + _config->debug_buffer_dump_file() + " for writing");
        return;
    }
    fs.write(reinterpret_cast<char*>(_debug_buffer->SystemPtr()), _debug_buffer->Size());

    _logger->info("Debug buffer has been written to " + _config->debug_buffer_dump_file());
}

hsa_status_t DebugAgent::intercept_hsa_code_object_reader_create_from_memory(
    decltype(hsa_code_object_reader_create_from_memory)* intercepted_fn,
    const void* code_object,
    size_t size,
    hsa_code_object_reader_t* code_object_reader)
{
    auto co = _code_object_manager->InitCodeObject(code_object, size, code_object_reader);
    _code_object_manager->WriteCodeObject(co);

    auto replacement_co = _code_object_swapper->get_swapped_code_object(*co, _debug_buffer);
    if (replacement_co)
        return intercepted_fn(replacement_co->Ptr(), replacement_co->Size(), code_object_reader);

    return intercepted_fn(code_object, size, code_object_reader);
}

hsa_status_t find_region_callback(
    hsa_region_t region,
    void* data)
{
    auto agent = reinterpret_cast<DebugAgent*>(data);

    hsa_region_segment_t segment_id;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment_id);
    if (segment_id == HSA_REGION_SEGMENT_GLOBAL)
    {
        hsa_region_global_flag_t flags;
        hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);

        if (flags & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED)
            agent->set_system_region(region);
        if (flags & HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED)
        {
            bool host_accessible_region = false;
            hsa_region_get_info(region, (hsa_region_info_t)HSA_AMD_REGION_INFO_HOST_ACCESSIBLE, &host_accessible_region);

            if (!host_accessible_region)
                agent->set_gpu_region(region);
        }
    }

    return HSA_STATUS_SUCCESS;
}

hsa_status_t DebugAgent::intercept_hsa_queue_create(
    decltype(hsa_queue_create)* intercepted_fn,
    hsa_agent_t agent,
    uint32_t size,
    hsa_queue_type32_t type,
    void (*callback)(hsa_status_t status, hsa_queue_t* source, void* data),
    void* data,
    uint32_t private_segment_size,
    uint32_t group_segment_size,
    hsa_queue_t** queue)
{
    hsa_status_t status = intercepted_fn(
        agent, size, type, callback, data, private_segment_size, group_segment_size, queue);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    status = hsa_agent_iterate_regions(agent, find_region_callback, this);
    if (status != HSA_STATUS_SUCCESS || _gpu_local_region.handle == 0 || _system_region.handle == 0)
    {
        _logger->error("Unable to find GPU region to allocate debug buffer");
        return status;
    }

    void* local_ptr;
    status = hsa_memory_allocate(_gpu_local_region, _config->debug_buffer_size(), &local_ptr);
    if (status != HSA_STATUS_SUCCESS)
    {
        _logger->error("Unable to allocate GPU local memory for debug buffer");
        return status;
    }

    void* system_ptr;
    status = hsa_memory_allocate(_system_region, _config->debug_buffer_size(), &system_ptr);
    if (status != HSA_STATUS_SUCCESS)
    {
        _logger->error("Unable to allocate system memory for debug buffer");
        return status;
    }
    _debug_buffer = std::make_unique<Buffer>(_config->debug_buffer_size(), local_ptr, system_ptr);

    std::ostringstream msg;
    msg << "Allocated debug buffer of size " << _config->debug_buffer_size() << " at " << _debug_buffer->LocalPtr();
    _logger->info(msg.str());

    return status;
}

hsa_status_t DebugAgent::intercept_hsa_executable_load_agent_code_object(
    decltype(hsa_executable_load_agent_code_object)* intercepted_fn,
    hsa_executable_t executable,
    hsa_agent_t agent,
    hsa_code_object_reader_t code_object_reader,
    const char* options,
    hsa_loaded_code_object_t* loaded_code_object)
{
    hsa_status_t status = intercepted_fn(executable, agent, code_object_reader, options, loaded_code_object);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    _code_object_manager->find_code_object_symbols(executable, code_object_reader);
    return status;
}
