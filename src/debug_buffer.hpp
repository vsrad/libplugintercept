#pragma once

#define AMD_INTERNAL_BUILD
#include <hsa_api_trace.h>

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
    size_t _size;
    void* _gpu_ptr;
    void* _host_ptr;

    static hsa_status_t iterate_memory_regions(hsa_region_t region, void* data);

public:
    DebugBuffer() : _size(0), _gpu_ptr{nullptr}, _host_ptr{nullptr} {}
    DebugBuffer(hsa_agent_t agent, AgentLogger& logger, uint64_t buffer_size);
    // Owns the buffer, should not be copied
    DebugBuffer(const DebugBuffer&) = delete;

    void write_to_file(AgentLogger& logger, const std::string& path);
    size_t size() const { return _size; }
    std::optional<size_t> gpu_buffer_address() const
    {
        if (_gpu_ptr)
            return {reinterpret_cast<size_t>(_gpu_ptr)};
        return {};
    }
};

} // namespace agent
