#include "buffer_manager.hpp"

using namespace agent;

void BufferManager::allocate_buffers(hsa_agent_t agent)
{
    if (_requested_allocs.empty())
        return;
    // allocation already performed
    if (_gpu_region.handle != 0)
        return;

    hsa_status_t status = hsa_agent_iterate_regions(agent, iterate_memory_regions, this);
    if (status != HSA_STATUS_SUCCESS)
    {
        _logger.hsa_error("Unable to allocate GPU buffers", status, "hsa_agent_iterate_regions");
        return;
    }
    if (_gpu_region.handle == 0 || _host_region.handle == 0)
    {
        _logger.error("Unable to find HSA memory regions to allocate buffers");
        return;
    }

    for (const auto& alloc : _requested_allocs)
    {
        BufferPointer ptr;

        status = hsa_memory_allocate(_gpu_region, alloc.size, &ptr.gpu);
        if (status != HSA_STATUS_SUCCESS)
        {
            _logger.hsa_error("Unable to allocate a buffer on GPU", status, "hsa_memory_allocate");
            return;
        }
        status = hsa_memory_allocate(_host_region, alloc.size, &ptr.host);
        if (status != HSA_STATUS_SUCCESS)
        {
            _logger.hsa_error("Unable to allocate host memory for a buffer", status, "hsa_memory_allocate");
            return;
        }

        _alloc_pointers.push_back(ptr);

        std::ostringstream msg;
        msg << "Allocated buffer of size " << alloc.size << " at " << ptr.gpu;
        _logger.info(msg.str());

        if (!alloc.addr_env_name.empty())
            _buffer_env.emplace_back(alloc.addr_env_name, std::to_string(reinterpret_cast<size_t>(ptr.gpu)));
        if (!alloc.size_env_name.empty())
            _buffer_env.emplace_back(alloc.size_env_name, std::to_string(alloc.size));
    }
}

BufferManager::~BufferManager()
{
    auto alloc = _requested_allocs.begin();
    auto ptr = _alloc_pointers.begin();
    for (; alloc != _requested_allocs.end() && ptr != _alloc_pointers.end(); alloc++, ptr++)
    {
        if (alloc->dump_path.empty())
            continue;

        std::ostringstream msg;

        hsa_status_t status = hsa_memory_copy(ptr->host, ptr->gpu, alloc->size);
        if (status != HSA_STATUS_SUCCESS)
        {
            msg << "Unable to copy buffer " << ptr->gpu << " from GPU";
            _logger.hsa_error(msg.str(), status, "hsa_memory_copy");
            return;
        }

        if (std::ofstream fs{alloc->dump_path, std::ios::out | std::ios::binary})
        {
            fs.write(reinterpret_cast<char*>(ptr->host), alloc->size);
            msg << "Buffer " << ptr->gpu << " has been written to " << alloc->dump_path;
            _logger.info(msg.str());
        }
        else
        {
            msg << "Failed to open " << alloc->dump_path << " to dump buffer " << ptr->gpu;
            _logger.error(msg.str());
        }
    }
}

hsa_status_t BufferManager::iterate_memory_regions(hsa_region_t region, void* data)
{
    auto self = reinterpret_cast<BufferManager*>(data);

    hsa_region_segment_t segment_id;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment_id);
    if (segment_id == HSA_REGION_SEGMENT_GLOBAL)
    {
        hsa_region_global_flag_t flags;
        hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);

        if (flags & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED)
            self->_host_region = region;
        if (flags & HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED)
        {
            bool host_accessible_region = false;
            hsa_region_get_info(region, (hsa_region_info_t)HSA_AMD_REGION_INFO_HOST_ACCESSIBLE, &host_accessible_region);

            if (!host_accessible_region)
                self->_gpu_region = region;
        }
    }
    return HSA_STATUS_SUCCESS;
}
