#include <amd_hsa_kernel_code.h>
#include <hsakmt.h>

#include "trap_handler.hpp"

using namespace agent;

void TrapHandler::set_up(hsa_agent_t agent)
{
    if (_config.code_object_path.empty())
        return;
    auto handler_co = CodeObject::try_read_from_file(_config.code_object_path.c_str());
    if (!handler_co)
    {
        _logger.error("Unable to read trap handler code object at " + _config.code_object_path);
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
        return;
    }

    if (_config.buffer_size % 4096 != 0)
    {
        _logger.error("Unable to allocate trap handler buffer: requested size (" + std::to_string(_config.buffer_size)
            + ") needs to be a multiple of page size (4096)");
    }
    else if (_config.buffer_size != 0)
    {
        HsaMemFlags flags = {0};
        flags.ui32.PageSize = HSA_PAGE_SIZE_4KB;
        flags.ui32.HostAccess = 1;
        flags.ui32.NoNUMABind = 1;
        HSAKMT_STATUS kmt_status = hsaKmtAllocMemory(_agent_node_id, _config.buffer_size, flags, &_trap_handler_buffer);
        const char* kmt_error_site = "hsaKmtAllocMemory";
        if (kmt_status == HSAKMT_STATUS_SUCCESS)
        {
            kmt_status = hsaKmtMapMemoryToGPUNodes(
                _trap_handler_buffer, _config.buffer_size, nullptr, {0}, 1, &_agent_node_id);
            kmt_error_site = "hsaKmtMapMemoryToGPUNodes";
        }
        if (kmt_status != HSAKMT_STATUS_SUCCESS)
        {
            _logger.hsa_kmt_error("Unable to allocate trap handler buffer", kmt_status, kmt_error_site);
            _trap_handler_buffer = nullptr;
        }
    }

    void* handler_entry = reinterpret_cast<void*>(kernel_sym_handle + sizeof(amd_kernel_code_t));
    HSAKMT_STATUS kmt_status;
    if (_trap_handler_buffer)
        kmt_status = hsaKmtSetTrapHandler(_agent_node_id, handler_entry, 0, _trap_handler_buffer, _config.buffer_size);
    else
        kmt_status = hsaKmtSetTrapHandler(_agent_node_id, handler_entry, 0, nullptr, 0);

    _handler_loaded = kmt_status == HSAKMT_STATUS_SUCCESS;
    if (_handler_loaded)
        _logger.info(
            "Successfully set trap handler kernel " + _config.symbol_name + " from " + _config.code_object_path);
    else
        _logger.hsa_kmt_error("Unable to register trap handler", kmt_status, "hsaKmtSetTrapHandler");
}

TrapHandler::~TrapHandler()
{
    if (_handler_loaded)
    {
        HSAKMT_STATUS kmt_status = hsaKmtSetTrapHandler(_agent_node_id, nullptr, 0, nullptr, 0);
        if (kmt_status != HSAKMT_STATUS_SUCCESS)
            _logger.hsa_kmt_error("Unable to reset trap handler", kmt_status, "hsaKmtSetTrapHandler");
    }
    if (_trap_handler_buffer)
    {
        HSAKMT_STATUS kmt_status = hsaKmtUnmapMemoryToGPU(_trap_handler_buffer);
        const char* kmt_error_site = "hsaKmtUnmapMemoryToGPU";
        if (kmt_status == HSAKMT_STATUS_SUCCESS)
        {
            kmt_status = hsaKmtFreeMemory(_trap_handler_buffer, _config.buffer_size);
            kmt_error_site = "hsaKmtFreeMemory";
        }
        if (kmt_status != HSAKMT_STATUS_SUCCESS)
            _logger.hsa_kmt_error("Unable to free trap handler buffer", kmt_status, kmt_error_site);
    }
}
