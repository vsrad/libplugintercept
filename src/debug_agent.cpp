#include "debug_agent.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace agent;

hsa_status_t DebugAgent::intercept_hsa_executable_symbol_get_info(
    decltype(hsa_executable_symbol_get_info)* intercepted_fn,
    hsa_executable_symbol_t executable_symbol,
    hsa_executable_symbol_info_t attribute,
    void* value)
{
    std::scoped_lock(_agent_mutex);
    auto call_id = ++_get_symbol_info_id;
    auto symbol_info = _co_recorder->record_get_info(executable_symbol, attribute, call_id);
    switch (attribute)
    {
    case HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH:
    case HSA_EXECUTABLE_SYMBOL_INFO_NAME:
    case HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME_LENGTH:
    case HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME:
        // The source symbol name may differ from the replacement name; return the former because host code may rely on it.
        return intercepted_fn(executable_symbol, attribute, value);
    default:
        if (symbol_info)
            if (auto replacement_sym = _co_substitutor->substitute_symbol(*symbol_info))
                return intercepted_fn(*replacement_sym, attribute, value);
        return intercepted_fn(executable_symbol, attribute, value);
    }
}

void DebugAgent::record_co_load(hsaco_t hsaco, const void* contents, size_t size, hsa_status_t load_status)
{
    _co_recorder->record_code_object(contents, size, hsaco, load_status);
}

hsa_status_t DebugAgent::executable_load_co(hsaco_t hsaco, hsa_agent_t agent, hsa_executable_t executable, std::function<hsa_status_t(hsaco_t)> loader)
{
    std::scoped_lock(_agent_mutex);
    if (_first_executable_load)
    {
        _first_executable_load = false;
        _buffer_manager->allocate_buffers(agent);
        ExternalCommand::run_init_command(_config->init_command(), _buffer_manager->buffer_environment_variables(), *_logger);
        _trap_handler->set_up(agent);
        _co_substitutor->prepare_symbol_substitutes(agent);
    }

    auto co = _co_recorder->find_code_object(&hsaco);
    if (!co)
        return loader(hsaco);

    const char* error_callsite;
    hsa_status_t status = HSA_STATUS_SUCCESS;
    if (auto sub = _co_substitutor->substitute(agent, co->get()))
    {
        status = _co_loader->load_from_memory(&hsaco, *sub, &error_callsite);
        if (status == HSA_STATUS_SUCCESS)
            /* Load the original executable as well to iterate its symbols */
            status = _co_loader->create_executable(co->get(), agent, &executable, &error_callsite);
        else
            _logger->hsa_error("Failed to load the replacement for CO " + co->get().info(), status, error_callsite);
    }
    if (status == HSA_STATUS_SUCCESS)
        status = loader(hsaco);

    // Once the executable is loaded, we can iterate its symbols
    exec_symbols_t symbols;
    if (status == HSA_STATUS_SUCCESS)
        status = _co_loader->enum_executable_symbols(executable, symbols, &error_callsite);
    if (status == HSA_STATUS_SUCCESS)
        _co_recorder->record_symbols(co->get(), std::move(symbols));
    else
        _logger->hsa_error("Failed to create an executable from " + co->get().info(), status, error_callsite);

    return status;
}
