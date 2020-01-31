#include "debug_agent.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace agent;

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

hsa_status_t DebugAgent::intercept_hsa_executable_load_agent_code_object(
    decltype(hsa_executable_load_agent_code_object)* intercepted_fn,
    hsa_executable_t executable,
    hsa_agent_t agent,
    hsa_code_object_reader_t code_object_reader,
    const char* options,
    hsa_loaded_code_object_t* loaded_code_object)
{
    auto co = _co_recorder->find_code_object(code_object_reader);
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
    if (!_debug_buffer)
        _debug_buffer = std::make_unique<DebugBuffer>(agent, *_logger, _config->debug_buffer_size());
    if (auto replacement_co = _co_swapper->swap_code_object(co, *_debug_buffer, agent))
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
    return {};
}

void DebugAgent::write_debug_buffer_to_file()
{
    if (_config->debug_buffer_dump_file().empty())
    {
        _logger->warning("Debug buffer dump file is not specified (debug-buffer.dump-file)");
        return;
    }
    if (!_debug_buffer)
    {
        _logger->info("Debug buffer hasn't been allocated, nothing will be written to " + _config->debug_buffer_dump_file());
        return;
    }
    _debug_buffer->write_to_file(*_logger, _config->debug_buffer_dump_file());
}
