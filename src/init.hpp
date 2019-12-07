#ifndef INIT_HPP__
#define INIT_HPP__

#define AMD_INTERNAL_BUILD
#include <hsa_api_trace.h>

// HSA Runtime function table without intercepts.
CoreApiTable _hsa_core_api_table;

class Buffer
{
private:
    size_t size;
    void* ptr;

public:
    Buffer(size_t size_, void* ptr_)
        : size{size_},
          ptr{ptr_} {}
    template <typename T>
    T* Ptr() { return (T*)ptr; }
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

#endif // INIT_HPP__