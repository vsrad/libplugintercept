#include "DebugAgent.hpp"
#include "init.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace agent;

DebugAgent::DebugAgent() : _gpu_local_region{0}, _system_region{0}
{
    auto debug_path = getenv("ASM_DBG_PATH");
    auto write_path = getenv("ASM_DBG_WRITE_PATH");
    auto debug_size_env = getenv("ASM_DBG_BUF_SIZE");

    if (!debug_path || !debug_size_env || !write_path)
    {
        std::ostringstream exception_message;
        exception_message << "Error: environment variable is not set." << std::endl
                          << "-- ASM_DBG_PATH: " << _debug_path << std::endl
                          << "-- ASM_DBG_BUF_SIZE: " << debug_size_env << std::endl;

        throw std::invalid_argument(exception_message.str());
    }

    _debug_path = debug_path;
    _debug_size = atoi(debug_size_env);

    auto write_path_str = std::string(write_path);
    _code_object_manager = std::make_unique<CodeObjectManager>(write_path_str);
}

void DebugAgent::write_debug_buffer_to_file()
{
    if (!_debug_buffer)
        std::cerr << "Error: debug buffer is not allocated" << std::endl;

    hsa_status_t status;
    status = hsa_memory_copy(_debug_buffer->SystemPtr(), _debug_buffer->LocalPtr(), _debug_buffer->Size());
    if (status != HSA_STATUS_SUCCESS)
    {
        std::cerr << "Unable to copy GPU local memory to system memory for debug buffer" << std::endl;
        return;
    }

    std::ofstream fs(_debug_path, std::ios::out | std::ios::binary);
    if (!fs.is_open())
    {
        std::cerr << "Failed to open " << _debug_path << std::endl;
        return;
    }

    std::cout << "Dump debug buffer to the file: " << _debug_path << std::endl;

    fs.write((char*)(_debug_buffer->SystemPtr()), _debug_buffer->Size());
    fs.close();
}

hsa_status_t DebugAgent::intercept_hsa_code_object_reader_create_from_memory(
    decltype(hsa_code_object_reader_create_from_memory)* intercepted_fn,
    const void* code_object,
    size_t size,
    hsa_code_object_reader_t* code_object_reader)
{
    uint32_t* patched_code_object = new uint32_t[size / 4];
    uint32_t* patched_code_object_end = patched_code_object + size / 4;
    memcpy(patched_code_object, code_object, size);

    auto co = _code_object_manager->InitCodeObject(code_object, size);
    _code_object_manager->WriteCodeObject(co);

    // s_mov_b32 encoding (Vega ISA):
    // bits 31-23 (0xff800000) are set to 0xbe8
    // bits 22-16 (0x007f0000) encode the destination register, don't check them
    // bits 15-8 (0x0000ff00) are set to 0
    // bits 7-0 (0x000000ff) encode the source operand, should be set to 0xff for an address literal
    const uint32_t s_mov_mask = 0xff80ffff;
    const uint32_t s_mov_pattern = 0xbe8000ff;
    const uint32_t address_literal_pattern = 0x7f7f7f7f;

    const size_t amd_kernel_code_t_offset = 256 / 4;
    for (uint32_t* instructions = patched_code_object + amd_kernel_code_t_offset;
         instructions < patched_code_object_end - 3; ++instructions)
    {
        if ((instructions[1] == address_literal_pattern) &&
            (instructions[3] == address_literal_pattern) &&
            (instructions[0] & s_mov_mask) == s_mov_pattern && // first s_mov_b32 instruction
            (instructions[2] & s_mov_mask) == s_mov_pattern)   // second s_mov_b32 instruction
        {
            uint64_t buf_addr = reinterpret_cast<uint64_t>(_debug_buffer->LocalPtr());
            instructions[1] = static_cast<uint32_t>(buf_addr & 0xffffffff);
            instructions[3] = static_cast<uint32_t>(buf_addr >> 32);
            std::cout << "Injected debug buffer address into code object at " << instructions << std::endl;
            instructions += 4;
        }
    }

    return intercepted_fn(patched_code_object, size, code_object_reader);
}

hsa_status_t find_region_callback(
    hsa_region_t region,
    void* data)
{
    auto agent = reinterpret_cast<DebugAgent*>(data);

    hsa_region_segment_t segment_id;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment_id);
    if (segment_id == HSA_REGION_SEGMENT_GLOBAL)
    {
        hsa_region_global_flag_t flags;
        hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);

        if (flags & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED)
            agent->set_system_region(region);
        if (flags & HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED)
        {
            bool host_accessible_region = false;
            hsa_region_get_info(region, (hsa_region_info_t)HSA_AMD_REGION_INFO_HOST_ACCESSIBLE, &host_accessible_region);

            if (!host_accessible_region)
                agent->set_gpu_region(region);
        }
    }

    return HSA_STATUS_SUCCESS;
}

hsa_status_t DebugAgent::intercept_hsa_queue_create(
    decltype(hsa_queue_create)* intercepted_fn,
    hsa_agent_t agent,
    uint32_t size,
    hsa_queue_type32_t type,
    void (*callback)(hsa_status_t status, hsa_queue_t* source, void* data),
    void* data,
    uint32_t private_segment_size,
    uint32_t group_segment_size,
    hsa_queue_t** queue)
{
    hsa_status_t status = intercepted_fn(
        agent, size, type, callback, data, private_segment_size, group_segment_size, queue);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    status = hsa_agent_iterate_regions(agent, find_region_callback, this);
    if (status != HSA_STATUS_SUCCESS || _gpu_local_region.handle == 0 || _system_region.handle == 0)
    {
        std::cerr << "Unable to find GPU region to allocate debug buffer" << std::endl;
        return status;
    }

    void* local_ptr;
    status = hsa_memory_allocate(_gpu_local_region, _debug_size, &local_ptr);
    if (status != HSA_STATUS_SUCCESS)
    {
        std::cerr << "Unable to allocate GPU local memory for debug buffer" << std::endl;
        return status;
    }

    void* system_ptr;
    status = hsa_memory_allocate(_system_region, _debug_size, &system_ptr);
    if (status != HSA_STATUS_SUCCESS)
    {
        std::cerr << "Unable to allocate system memory for debug buffer" << std::endl;
        return status;
    }
    _debug_buffer = std::make_unique<Buffer>(_debug_size, local_ptr, system_ptr);
    std::cout << "Allocated debug buffer of size " << _debug_size << " at " << _debug_buffer->LocalPtr() << std::endl;

    return status;
}
