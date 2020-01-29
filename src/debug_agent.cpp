#include "debug_agent.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace agent;

void DebugAgent::write_debug_buffer_to_file()
{
    if (!_debug_buffer)
    {
        _logger->info("Debug buffer is not allocated");
        return;
    }

    hsa_status_t status;
    status = hsa_memory_copy(_debug_buffer->SystemPtr(), _debug_buffer->LocalPtr(), _debug_buffer->Size());
    if (status != HSA_STATUS_SUCCESS)
    {
        _logger->error("Unable to copy GPU local memory to system memory for debug buffer");
        return;
    }

    std::ofstream fs(_config->debug_buffer_dump_file(), std::ios::out | std::ios::binary);
    if (!fs.is_open())
    {
        _logger->error("Failed to open " + _config->debug_buffer_dump_file() + " for dumping debug buffer");
        return;
    }
    fs.write(reinterpret_cast<char*>(_debug_buffer->SystemPtr()), _debug_buffer->Size());

    _logger->info("Debug buffer has been written to " + _config->debug_buffer_dump_file());
}

hsa_status_t DebugAgent::intercept_hsa_code_object_reader_create_from_memory(
    decltype(hsa_code_object_reader_create_from_memory)* intercepted_fn,
    const void* code_object,
    size_t size,
    hsa_code_object_reader_t* code_object_reader)
{
    auto& co = _co_recorder->record_code_object(code_object, size);
    hsa_status_t status = intercepted_fn(code_object, size, code_object_reader);
    if (status == HSA_STATUS_SUCCESS && code_object_reader->handle != 0)
        co.set_hsa_code_object_reader(*code_object_reader);
    return status;
}

hsa_status_t DebugAgent::intercept_hsa_code_object_deserialize(
    decltype(hsa_code_object_deserialize)* intercepted_fn,
    void* serialized_code_object,
    size_t serialized_code_object_size,
    const char* options,
    hsa_code_object_t* code_object)
{
    auto& co = _co_recorder->record_code_object(serialized_code_object, serialized_code_object_size);
    hsa_status_t status = intercepted_fn(serialized_code_object, serialized_code_object_size, options, code_object);
    if (status == HSA_STATUS_SUCCESS && code_object->handle != 0)
        co.set_hsa_code_object(*code_object);
    return status;
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

    if (_debug_buffer)
    {
        _logger->warning("hsa_queue_create is called multiple times, debug buffer allocation may be invalid");
        return status;
    }
    if (_config->debug_buffer_size() == 0)
    {
        _logger->warning("Debug buffer will not be allocated (debug buffer set is set to 0)");
        return status;
    }

    status = hsa_agent_iterate_regions(agent, find_region_callback, this);
    if (status != HSA_STATUS_SUCCESS || _gpu_local_region.handle == 0 || _system_region.handle == 0)
    {
        _logger->error("Unable to find GPU region to allocate debug buffer");
        return status;
    }

    void* local_ptr;
    status = hsa_memory_allocate(_gpu_local_region, _config->debug_buffer_size(), &local_ptr);
    if (status != HSA_STATUS_SUCCESS)
    {
        _logger->error("Unable to allocate GPU local memory for debug buffer");
        return status;
    }

    void* system_ptr;
    status = hsa_memory_allocate(_system_region, _config->debug_buffer_size(), &system_ptr);
    if (status != HSA_STATUS_SUCCESS)
    {
        _logger->error("Unable to allocate system memory for debug buffer");
        return status;
    }
    _debug_buffer = std::make_unique<Buffer>(_config->debug_buffer_size(), local_ptr, system_ptr);

    std::ostringstream msg;
    msg << "Allocated debug buffer of size " << _config->debug_buffer_size() << " at " << _debug_buffer->LocalPtr();
    _logger->info(msg.str());

    return status;
}

hsa_status_t DebugAgent::intercept_hsa_executable_load_agent_code_object(
    decltype(hsa_executable_load_agent_code_object)* intercepted_fn,
    hsa_executable_t executable,
    hsa_agent_t agent,
    hsa_code_object_reader_t code_object_reader,
    const char* options,
    hsa_loaded_code_object_t* loaded_code_object)
{
    auto co = _co_recorder->find_code_object(code_object_reader);
    if (auto replacement_co = _co_swapper->swap_code_object(co->get(), _debug_buffer))
        if (auto replacement_reader = load_swapped_code_object<hsa_code_object_reader_t>(agent, co->get()))
            return intercepted_fn(executable, agent, *replacement_reader, options, loaded_code_object);

    hsa_status_t status = intercepted_fn(executable, agent, code_object_reader, options, loaded_code_object);
    if (co && status == HSA_STATUS_SUCCESS)
        _co_recorder->iterate_symbols(executable, co->get());
    return status;
}

hsa_status_t DebugAgent::intercept_hsa_executable_load_code_object(
    decltype(hsa_executable_load_code_object)* intercepted_fn,
    hsa_executable_t executable,
    hsa_agent_t agent,
    hsa_code_object_t code_object,
    const char* options)
{
    auto co = _co_recorder->find_code_object(code_object);
    if (auto replacement_co = _co_swapper->swap_code_object(co->get(), _debug_buffer))
        if (auto replacement_hsaco = load_swapped_code_object<hsa_code_object_t>(agent, co->get()))
            return intercepted_fn(executable, agent, *replacement_hsaco, options);

    hsa_status_t status = intercepted_fn(executable, agent, code_object, options);
    if (co && status == HSA_STATUS_SUCCESS)
        _co_recorder->iterate_symbols(executable, co->get());
    return status;
}

hsa_status_t DebugAgent::intercept_hsa_executable_symbol_get_info(
    decltype(hsa_executable_symbol_get_info)* intercepted_fn,
    hsa_executable_symbol_t executable_symbol,
    hsa_executable_symbol_info_t attribute,
    void* value)
{
    switch (attribute)
    {
    case HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH:
    case HSA_EXECUTABLE_SYMBOL_INFO_NAME:
    case HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME_LENGTH:
    case HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME:
        // The source symbol name may differ from the replacement name; return the former because host code may rely on it.
        return intercepted_fn(executable_symbol, attribute, value);
    default:
        if (auto replacement_sym{_co_swapper->swap_symbol(executable_symbol)})
            return intercepted_fn(*replacement_sym, attribute, value);
        return intercepted_fn(executable_symbol, attribute, value);
    }
}

template <typename T>
std::optional<T> DebugAgent::load_swapped_code_object(hsa_agent_t agent, RecordedCodeObject& co)
{
    if (auto replacement_co = _co_swapper->swap_code_object(co, _debug_buffer))
    {
        T loaded_replacement;
        const char* error_callsite;
        hsa_status_t status = _co_loader->load_from_memory(*replacement_co, &loaded_replacement, &error_callsite);
        if (status == HSA_STATUS_SUCCESS)
        {
            /* Load the original executable to iterate its symbols */
            hsa_executable_t original_exec;
            const char* error_callsite;
            hsa_status_t status = _co_loader->create_executable(co, agent, &original_exec, &error_callsite);
            if (status == HSA_STATUS_SUCCESS)
                _co_recorder->iterate_symbols(original_exec, co);
            else
                _logger->hsa_error("Unable to load executable with CRC = " + std::to_string(co.crc()), status, error_callsite);

            return {loaded_replacement};
        }
        else
        {
            _logger->hsa_error("Failed to load replacement code object for CRC = " + std::to_string(co.crc()), status, error_callsite);
        }
    }
    else
    {
        _co_swapper->prepare_symbol_swap(co, *_co_loader, agent);
    }
    return {};
}
