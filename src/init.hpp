#pragma once

#define AMD_INTERNAL_BUILD
#include <hsa_api_trace.h>

#include "CRC.h"

// HSA Runtime function table without intercepts.
CoreApiTable _hsa_core_api_table;
hsa_region_t _gpu_local_region {0};
hsa_region_t _system_region {0};

const char* _debug_path;
size_t _debug_size;

CRC::Table<uint32_t, 32> _crc_table(CRC::CRC_32());

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

Buffer* _debug_buffer = nullptr;

hsa_status_t find_gpu_region_callback(
    hsa_region_t region,
    void* data);

/*
*  HSA api interceptors
*/
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
