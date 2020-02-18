#include <hsa.h>

#include "code_object_loader.hpp"
#include "logger/logger.hpp"

namespace agent
{
class TrapHandler
{
private:
    AgentLogger& _logger;
    CodeObjectLoader& _co_loader;
    std::optional<CodeObject> _loaded_handler;
    uint32_t _agent_node_id = 0;

public:
    TrapHandler(AgentLogger& logger, CodeObjectLoader& co_loader)
        : _logger(logger), _co_loader(co_loader) {}
    ~TrapHandler();
    void load_handler(CodeObject&& handler_co, hsa_agent_t agent);
};
} // namespace agent
