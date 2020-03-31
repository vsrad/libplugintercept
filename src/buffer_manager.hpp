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

class BufferManager
{
private:
    const std::vector<BufferAllocation>& _requested_allocs;
    AgentLogger& _logger;

    hsa_region_t _gpu_region{0};
    hsa_region_t _host_region{0};
    std::vector<BufferPointer> _alloc_pointers;
    ext_environment_t _buffer_env;

    static hsa_status_t iterate_memory_regions(hsa_region_t region, void* data);

public:
    BufferManager(const std::vector<BufferAllocation>& allocs, AgentLogger& logger)
        : _requested_allocs(allocs), _logger(logger) {}
    ~BufferManager();

    void allocate_buffers(hsa_agent_t agent);
    const ext_environment_t& buffer_environment_variables() const { return _buffer_env; }
};
} // namespace agent
