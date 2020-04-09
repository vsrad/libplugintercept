#include <hsakmt.h>

#include "trap_handler.hpp"

using namespace agent;

void TrapHandler::set_up(hsa_agent_t agent)
{
    if (_state != TrapHandlerState::None || _config.code_object_path.empty())
        return;
    auto handler_co = CodeObject::try_read_from_file(_config.code_object_path.c_str());
    if (!handler_co)
    {
        _logger.error("Unable to read trap handler code object at " + _config.code_object_path);
        _state = TrapHandlerState::FailedToLoad;
        return;
    }

    uint64_t kernel_sym_handle;
    hsa_executable_t handler_exec;
    hsa_executable_symbol_t kernel_sym;
    hsa_status_t status = hsa_agent_get_info(agent, HSA_AGENT_INFO_NODE, &_agent_node_id);
    const char* err_callsite = "hsa_agent_get_info";
    if (status == HSA_STATUS_SUCCESS)
        status = _co_loader.create_executable(*handler_co, agent, &handler_exec, &err_callsite);
    if (status == HSA_STATUS_SUCCESS)
        status = _co_loader.find_symbol(agent, handler_exec, _config.symbol_name.c_str(), &kernel_sym, &err_callsite);
    if (status == HSA_STATUS_SUCCESS)
        status = _co_loader.get_kernel_handle(kernel_sym, &kernel_sym_handle, &err_callsite);
    if (status != HSA_STATUS_SUCCESS)
    {
        _logger.hsa_error("Unable to get trap handler symbol handle: ", status, err_callsite);
        _state = TrapHandlerState::FailedToLoad;
        return;
    }

    void* handler_entry = (void*)(kernel_sym_handle + 256 /* sizeof(amd_kernel_code_t) */);
    HSAKMT_STATUS kmt_status = hsaKmtSetTrapHandler(_agent_node_id, handler_entry, 0, nullptr, 0);
    if (kmt_status == HSAKMT_STATUS_SUCCESS)
    {
        _logger.info("Successfully set " + _config.symbol_name + " from " + _config.code_object_path + " as the trap handler");
        _state = TrapHandlerState::Configured;
    }
    else
    {
        _logger.error("Unable to register trap handler: HSAKMT_STATUS = " + std::to_string(kmt_status));
        _state = TrapHandlerState::FailedToLoad;
    }
}

TrapHandler::~TrapHandler()
{
    if (_state != TrapHandlerState::Configured)
        return;
    HSAKMT_STATUS kmt_status = hsaKmtSetTrapHandler(_agent_node_id, nullptr, 0, nullptr, 0);
    if (kmt_status != HSAKMT_STATUS_SUCCESS)
        _logger.error("Unable to reset trap handler: HSAKMT_STATUS = " + std::to_string(kmt_status));
}
