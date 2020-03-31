#pragma once

#include "code_object_loader.hpp"
#include "code_object_swap.hpp"
#include "external_command.hpp"
#include "logger/logger.hpp"
#include <vector>

namespace agent
{
class CodeObjectSwapper
{
private:
    const std::vector<CodeObjectSwap>& _swaps;
    const std::vector<CodeObjectSymbolSubstitute>& _symbol_subs;
    AgentLogger& _logger;
    CodeObjectLoader& _co_loader;

    std::unordered_map<decltype(hsa_executable_symbol_t::handle), hsa_executable_symbol_t> _evaluated_symbol_subs;

public:
    CodeObjectSwapper(const std::vector<CodeObjectSwap>& swaps,
                      const std::vector<CodeObjectSymbolSubstitute>& symbol_subs,
                      AgentLogger& logger, CodeObjectLoader& co_loader)
        : _swaps(swaps), _symbol_subs(symbol_subs), _logger(logger), _co_loader(co_loader) {}

    std::optional<CodeObject> try_swap(hsa_agent_t agent, const RecordedCodeObject& source, const ext_environment_t& env);
    void prepare_symbol_substitutes(hsa_agent_t agent, const RecordedCodeObject& source, const ext_environment_t& env);
    std::optional<hsa_executable_symbol_t> substitute_symbol(hsa_executable_symbol_t sym);
};
}; // namespace agent
