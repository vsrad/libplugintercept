#pragma once

#include "debug_buffer.hpp"
#include "code_object_loader.hpp"
#include "code_object_swap.hpp"
#include "logger/logger.hpp"
#include <vector>

namespace agent
{
class CodeObjectSwapper
{
private:
    const std::vector<CodeObjectSwap>& _swaps;
    AgentLogger& _logger;
    CodeObjectLoader& _co_loader;

    std::unordered_map<decltype(hsa_executable_symbol_t::handle), hsa_executable_symbol_t> _swapped_symbols;

    bool run_external_command(const std::string& cmd, const DebugBuffer& debug_buffer);
    static hsa_status_t map_swapped_symbols(hsa_executable_t exec, hsa_executable_symbol_t sym, void* data);

public:
    CodeObjectSwapper(const std::vector<CodeObjectSwap>& swaps, AgentLogger& logger, CodeObjectLoader& co_loader)
        : _swaps(swaps), _logger(logger), _co_loader(co_loader) {}

    std::optional<CodeObject> swap_code_object(
        const RecordedCodeObject& source,
        const DebugBuffer& debug_buffer,
        hsa_agent_t agent);
    std::optional<hsa_executable_symbol_t> swap_symbol(hsa_executable_symbol_t sym);
};
}; // namespace agent
