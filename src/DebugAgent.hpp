#pragma once

#define AMD_INTERNAL_BUILD
#include "CodeObjectManager.hpp"
#include <hsa_api_trace.h>
#include <memory>
#include <string>

namespace agent
{

class Buffer
{
private:
    size_t size;
    void *local_ptr, *system_ptr;

public:
    Buffer(size_t size_, void* local_ptr_, void* system_ptr_)
        : size(size_), local_ptr(local_ptr_), system_ptr(system_ptr_) {}
    void* LocalPtr() { return local_ptr; }
    void* SystemPtr() { return system_ptr; }
    size_t Size() const { return size; }
};

class DebugAgent
{
private:
    CoreApiTable _api_core_table;
    hsa_region_t _gpu_local_region;
    hsa_region_t _system_region;
    std::string _debug_path;
    size_t _debug_size;
    std::unique_ptr<CodeObjectManager> _code_object_manager;
    std::unique_ptr<Buffer> _debug_buffer;

public:
    DebugAgent(HsaApiTable& api_table);

    hsa_region_t GetGpuRegion();
    hsa_region_t GetSystemRegion();

    void SetGpuRegion(hsa_region_t region);
    void SetSystemRegion(hsa_region_t region);

    void DebugBufferToFile();

    hsa_status_t intercept_hsa_code_object_reader_create_from_memory(
        const void* code_object,
        size_t size,
        hsa_code_object_reader_t* code_object_reader);

    hsa_status_t intercept_hsa_queue_create(
        hsa_agent_t agent,
        uint32_t size,
        hsa_queue_type32_t type,
        void (*callback)(hsa_status_t status, hsa_queue_t* source, void* data),
        void* data,
        uint32_t private_segment_size,
        uint32_t group_segment_size,
        hsa_queue_t** queue);
};

} // namespace agent