#pragma once

#define AMD_INTERNAL_BUILD
#include <hsa_api_trace.h>

#include "buffer.hpp"
#include "config.hpp"
#include "logger/logger.hpp"
#include <memory>

namespace agent
{
class DebugBuffer
{
private:
    hsa_region_t _gpu_region;
    hsa_region_t _host_region;
    std::unique_ptr<Buffer> _gpu_buffer;

    static hsa_status_t iterate_memory_regions(hsa_region_t region, void* data);

public:
    DebugBuffer(hsa_agent_t agent, AgentLogger& logger, uint64_t buffer_size);
    void write_to_file(AgentLogger& logger, const std::string& path);
    const std::unique_ptr<Buffer>& gpu_buffer() const { return _gpu_buffer; }
};

} // namespace agent
