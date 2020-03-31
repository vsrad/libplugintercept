#pragma once

#define AMD_INTERNAL_BUILD
#include <hsa_api_trace.h>

#include "buffer_allocation.hpp"
#include "external_command.hpp"
#include "logger/logger.hpp"
#include <vector>

namespace agent
{
struct BufferPointer
{
    void* gpu;
    void* host;
};

class BufferAllocator
{
private:
    hsa_region_t _gpu_region;
    hsa_region_t _host_region;
    std::vector<BufferPointer> _alloc_pointers;
    const std::vector<BufferAllocation>& _requested_allocs;
    AgentLogger& _logger;

    static hsa_status_t iterate_memory_regions(hsa_region_t region, void* data);

public:
    BufferAllocator(const std::vector<BufferAllocation>& allocs, AgentLogger& logger)
        : _gpu_region{0}, _host_region{0}, _requested_allocs(allocs), _logger(logger) {}
    void allocate_buffers(hsa_agent_t agent);
    void dump_buffers();
    ext_environment_t environment_variables() const;
};
} // namespace agent
