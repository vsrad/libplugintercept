#pragma once

#include "code_object_loader.hpp"
#include "code_object_swap.hpp"
#include "debug_buffer.hpp"
#include "logger/logger.hpp"
#include <vector>

namespace agent
{
struct SwapResult
{
    std::optional<CodeObject> replacement_co;
    std::optional<CodeObject> trap_handler_co;

    explicit operator bool() const { return bool(replacement_co); }
};

class CodeObjectSwapper
{
private:
    const std::vector<CodeObjectSwap>& _swaps;
    AgentLogger& _logger;
    CodeObjectLoader& _co_loader;

    std::unordered_map<decltype(hsa_executable_symbol_t::handle), hsa_executable_symbol_t> _swapped_symbols;

    bool run_external_command(const std::string& cmd, std::map<std::string, std::string> env);
    static hsa_status_t map_swapped_symbols(hsa_executable_t exec, hsa_executable_symbol_t sym, void* data);

public:
    CodeObjectSwapper(const std::vector<CodeObjectSwap>& swaps, AgentLogger& logger, CodeObjectLoader& co_loader)
        : _swaps(swaps), _logger(logger), _co_loader(co_loader) {}

    SwapResult try_swap(hsa_agent_t agent, const RecordedCodeObject& source, std::map<std::string, std::string> env);
    std::optional<hsa_executable_symbol_t> swap_symbol(hsa_executable_symbol_t sym);
};
}; // namespace agent
