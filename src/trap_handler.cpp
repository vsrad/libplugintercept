#include <hsakmt.h>

#include "trap_handler.hpp"

using namespace agent;

void TrapHandler::setup_handler(hsa_agent_t agent)
{
    auto handler_co = CodeObject::try_read_from_file("build/tests/kernels/trap_handler.co");
    if (!handler_co)
    {
      _logger.error("Unable to load trap handler");
      return;
    }


    uint64_t kernel_sym;
    const char *err_callsite, *err;
    hsa_executable_t handler_exec;
    hsa_status_t status = hsa_agent_get_info(agent, HSA_AGENT_INFO_NODE, &_agent_node_id);
    if (status != HSA_STATUS_SUCCESS)
        err_callsite = "hsa_agent_get_info";
    if (status == HSA_STATUS_SUCCESS)
        status = _co_loader.create_executable(*handler_co, agent, &handler_exec, &err_callsite);
    if (status == HSA_STATUS_SUCCESS)
        status = _co_loader.create_symbol_handle(agent, handler_exec, "trap_handler", &kernel_sym, &err_callsite);
    if (status != HSA_STATUS_SUCCESS)
    {
        hsa_status_string(status, &err);
        _logger.error(std::string("Unable to create a trap handler executable: ").append(err_callsite).append(" failed with ").append(err));
        return;
    }

    void* handler_entry = (void*)(kernel_sym + 256 /* sizeof(amd_kernel_code_t) */);
    HSAKMT_STATUS kmt_status = hsaKmtSetTrapHandler(_agent_node_id, handler_entry, 0, nullptr, 0);
    if (kmt_status != HSAKMT_STATUS_SUCCESS)
    {
        _logger.error("Unable to register trap handler: HSAKMT_STATUS = " + std::to_string(kmt_status));
        return;
    }
}

TrapHandler::~TrapHandler()
{
    if (_agent_node_id != 0)
        return;

    HSAKMT_STATUS kmt_status = hsaKmtSetTrapHandler(_agent_node_id, nullptr, 0, nullptr, 0);
    if (kmt_status != HSAKMT_STATUS_SUCCESS)
        _logger.error("Unable to reset trap handler: HSAKMT_STATUS = " + std::to_string(kmt_status));
}
