#pragma once
#include <cstring>
#include <hsa.h>
#include <iostream>

void hsa_error(const char* msg, hsa_status_t status)
{
    const char* err = 0;
    if (status != HSA_STATUS_SUCCESS)
        hsa_status_string(status, &err);
    std::cerr << msg << ": " << (err ? err : "unknown error") << std::endl;
    exit(1);
}

hsa_status_t find_gpu_device(hsa_agent_t agent, void* data)
{
    hsa_device_type_t dev;
    hsa_status_t status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &dev);
    if (status == HSA_STATUS_SUCCESS && dev == HSA_DEVICE_TYPE_GPU)
        *static_cast<hsa_agent_t*>(data) = agent;
    return HSA_STATUS_SUCCESS;
}

class KernelRunner
{
private:
    hsa_agent_t _gpu_agent;
    hsa_signal_t _completion_signal;
    hsa_queue_t* _queue;
    hsa_kernel_dispatch_packet_t* _dispatch_packet;
    uint64_t _dispatch_packet_index;
    uint64_t _code_object_handle;
    hsa_code_object_t _code_object;
    uint32_t _group_static_size;

public:
    KernelRunner()
    {
        hsa_status_t status = hsa_init();
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("hsa_init failed", status);

        status = hsa_iterate_agents(find_gpu_device, &_gpu_agent);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("hsa_iterate_agents failed", status);

        uint32_t queue_size;
        status = hsa_agent_get_info(_gpu_agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("Failed to get HSA_AGENT_INFO_QUEUE_MAX_SIZE", status);
        status = hsa_queue_create(_gpu_agent, queue_size, HSA_QUEUE_TYPE_MULTI, NULL, NULL, UINT32_MAX, UINT32_MAX, &_queue);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("hsa_queue_create failed", status);

        status = hsa_signal_create(1, 0, NULL, &_completion_signal);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("hsa_signal_create failed", status);
    }

    void load_code_object(const char* filename, const char* symbol_name)
    {
        std::ifstream in(filename, std::ios::binary | std::ios::ate);
        if (!in)
        {
            std::cerr << "Error: failed to load " << filename << std::endl;
            exit(1);
        }
        size_t size = std::string::size_type(in.tellg());
        char* ptr = (char*)malloc(size);
        in.seekg(0, std::ios::beg);
        std::copy(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), ptr);

        hsa_code_object_reader_t co_reader = {};
        hsa_status_t status = hsa_code_object_reader_create_from_memory(ptr, size, &co_reader);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("failed to deserialize code object", status);

        hsa_executable_t executable;
        status = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, NULL, &executable);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("hsa_executable_create failed", status);

        status = hsa_executable_load_agent_code_object(executable, _gpu_agent, co_reader, NULL, NULL);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("hsa_executable_load_agent_code_object failed", status);

        status = hsa_executable_freeze(executable, NULL);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("hsa_executable_freeze failed", status);

        hsa_executable_symbol_t symbol;
        status = hsa_executable_get_symbol(executable, NULL, symbol_name, _gpu_agent, 0, &symbol);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("hsa_executable_get_symbol failed", status);

        status = hsa_executable_symbol_get_info(
            symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, &_code_object_handle);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("hsa_executable_symbol_get_info failed", status);

        status = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE, &_group_static_size);
        if (status != HSA_STATUS_SUCCESS)
            hsa_error("hsa_executable_symbol_get_info failed", status);
    }

    void init_dispatch_packet(uint16_t workgroup_x, uint32_t grid_x)
    {
        const uint32_t queue_mask = _queue->size - 1;
        _dispatch_packet_index = hsa_queue_add_write_index_relaxed(_queue, 1);
        _dispatch_packet = static_cast<hsa_kernel_dispatch_packet_t*>(_queue->base_address) + (_dispatch_packet_index & queue_mask);
        memset((uint8_t*)_dispatch_packet + 4, 0, sizeof(*_dispatch_packet) - 4);
        _dispatch_packet->completion_signal = _completion_signal;
        _dispatch_packet->kernel_object = _code_object_handle;
        _dispatch_packet->workgroup_size_x = workgroup_x;
        _dispatch_packet->grid_size_x = grid_x;
    }

    void dispatch_kernel()
    {
        uint16_t header =
            (HSA_PACKET_TYPE_KERNEL_DISPATCH << HSA_PACKET_HEADER_TYPE) |
            (1 << HSA_PACKET_HEADER_BARRIER) |
            (HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE) |
            (HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE);
        uint16_t dim = 1;
        if (_dispatch_packet->grid_size_y > 1)
            dim = 2;
        if (_dispatch_packet->grid_size_z > 1)
            dim = 3;
        _dispatch_packet->group_segment_size = _group_static_size;
        uint16_t setup = dim << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
        uint32_t header32 = header | (setup << 16);
        __atomic_store_n((uint32_t*)_dispatch_packet, header32, __ATOMIC_RELEASE);
        hsa_signal_store_relaxed(_queue->doorbell_signal, _dispatch_packet_index);
    }

    void await_kernel_completion()
    {
        hsa_signal_value_t result = -1;
        while (result != HSA_STATUS_SUCCESS)
            result = hsa_signal_wait_acquire(_completion_signal, HSA_SIGNAL_CONDITION_EQ, 0, ~0ULL, HSA_WAIT_STATE_ACTIVE);
    }
};
