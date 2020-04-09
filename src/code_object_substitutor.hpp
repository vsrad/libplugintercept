#pragma once

#include "code_object_loader.hpp"
#include "config/code_object_substitute.hpp"
#include "external_command.hpp"
#include "logger/logger.hpp"
#include <vector>

namespace agent
{
class CodeObjectSubstitutor
{
private:
    const std::vector<config::CodeObjectSubstitute>& _subs;
    const std::vector<config::CodeObjectSymbolSubstitute>& _symbol_subs;
    AgentLogger& _logger;
    CodeObjectLoader& _co_loader;

    std::vector<std::pair<const config::CodeObjectSymbolSubstitute&, hsa_executable_symbol_t>> _evaluated_symbol_subs;

public:
    CodeObjectSubstitutor(const std::vector<config::CodeObjectSubstitute>& subs,
                          const std::vector<config::CodeObjectSymbolSubstitute>& symbol_subs,
                          AgentLogger& logger, CodeObjectLoader& co_loader)
        : _subs(subs), _symbol_subs(symbol_subs), _logger(logger), _co_loader(co_loader) {}

    std::optional<CodeObject> substitute(hsa_agent_t agent, const RecordedCodeObject& co);

    void prepare_symbol_substitutes(hsa_agent_t agent);

    std::optional<hsa_executable_symbol_t> substitute_symbol(
        get_info_call_id_t call_id,
        const RecordedCodeObject& co,
        hsa_executable_symbol_t sym);
};
}; // namespace agent
