#include "debug_buffer.hpp"

using namespace agent;

DebugBuffer::DebugBuffer(hsa_agent_t agent, AgentLogger& logger, uint64_t buffer_size)
{
    if (buffer_size == 0)
    {
        logger.warning("Debug buffer will not be allocated (debug buffer set is set to 0)");
        return;
    }

    hsa_status_t status = hsa_agent_iterate_regions(agent, iterate_memory_regions, this);
    if (status != HSA_STATUS_SUCCESS)
    {
        logger.hsa_error("Unable to allocate debug buffer", status, "hsa_agent_iterate_regions");
        return;
    }
    if (_gpu_region.handle == 0 || _host_region.handle == 0)
    {
        logger.error("Unable to find GPU region to allocate debug buffer");
        return;
    }

    void* local_ptr;
    status = hsa_memory_allocate(_gpu_region, buffer_size, &local_ptr);
    if (status != HSA_STATUS_SUCCESS)
    {
        logger.hsa_error("Unable to allocate GPU memory for debug buffer", status, "hsa_memory_allocate");
        return;
    }

    void* system_ptr;
    status = hsa_memory_allocate(_host_region, buffer_size, &system_ptr);
    if (status != HSA_STATUS_SUCCESS)
    {
        logger.hsa_error("Unable to allocate host memory for debug buffer", status, "hsa_memory_allocate");
        return;
    }

    _gpu_buffer = std::make_unique<Buffer>(buffer_size, local_ptr, system_ptr);

    std::ostringstream msg;
    msg << "Allocated debug buffer of size " << buffer_size << " at " << _gpu_buffer->LocalPtr();
    logger.info(msg.str());
}

void DebugBuffer::write_to_file(AgentLogger& logger, const std::string& path)
{
    if (!_gpu_buffer)
        return;

    hsa_status_t status;
    status = hsa_memory_copy(_gpu_buffer->SystemPtr(), _gpu_buffer->LocalPtr(), _gpu_buffer->Size());
    if (status != HSA_STATUS_SUCCESS)
    {
        logger.hsa_error("Unable to copy debug buffer from GPU", status, "hsa_memory_copy");
        return;
    }

    if (std::ofstream fs{path, std::ios::out | std::ios::binary})
    {
        fs.write(reinterpret_cast<char*>(_gpu_buffer->SystemPtr()), _gpu_buffer->Size());
        logger.info("Debug buffer has been written to " + path);
    }
    else
    {
        logger.error("Failed to open " + path + " for dumping debug buffer");
    }
}

hsa_status_t DebugBuffer::iterate_memory_regions(hsa_region_t region, void* data)
{
    auto this_ = reinterpret_cast<DebugBuffer*>(data);

    hsa_region_segment_t segment_id;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment_id);
    if (segment_id == HSA_REGION_SEGMENT_GLOBAL)
    {
        hsa_region_global_flag_t flags;
        hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);

        if (flags & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED)
            this_->_host_region = region;
        if (flags & HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED)
        {
            bool host_accessible_region = false;
            hsa_region_get_info(region, (hsa_region_info_t)HSA_AMD_REGION_INFO_HOST_ACCESSIBLE, &host_accessible_region);

            if (!host_accessible_region)
                this_->_gpu_region = region;
        }
    }
    return HSA_STATUS_SUCCESS;
}
