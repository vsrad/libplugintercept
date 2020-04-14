#pragma once

#include "code_object_loader.hpp"
#include "config/trap_handler.hpp"
#include "logger/logger.hpp"

namespace agent
{
class TrapHandler
{
private:
    AgentLogger& _logger;
    CodeObjectLoader& _co_loader;
    const config::TrapHandler& _config;

    bool _handler_loaded{false};
    uint32_t _agent_node_id = 0;

public:
    TrapHandler(AgentLogger& logger, CodeObjectLoader& co_loader, const config::TrapHandler& config)
        : _logger(logger), _co_loader(co_loader), _config(config) {}
    ~TrapHandler();
    void set_up(hsa_agent_t agent);
};
} // namespace agent
