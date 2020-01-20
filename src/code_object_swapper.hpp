#pragma once

#include "code_object_swap.hpp"
#include "code_object_loader.hpp"
#include "logger/logger.hpp"
#include "buffer.hpp"
#include <vector>

namespace agent
{
class CodeObjectSwapper
{
private:
    std::shared_ptr<std::vector<CodeObjectSwap>> _swaps;
    std::shared_ptr<AgentLogger> _logger;

    struct SymbolSwap
    {
        const CodeObjectSwap& swap;
        CodeObject replacement_co;
        hsa_executable_t exec;
    };
    std::unordered_map<call_count_t, SymbolSwap> _symbol_swaps;
    std::unordered_map<decltype(hsa_executable_symbol_t::handle), hsa_executable_symbol_t> _swapped_symbols;

    std::optional<CodeObject> do_swap(const CodeObjectSwap& swap, const RecordedCodeObject& source, const std::unique_ptr<Buffer>& debug_buffer);
    static hsa_status_t map_swapped_symbols(hsa_executable_t exec, hsa_executable_symbol_t sym, void* data);

public:
    CodeObjectSwapper(std::shared_ptr<std::vector<CodeObjectSwap>> swaps, std::shared_ptr<AgentLogger> logger)
        : _swaps(swaps), _logger(logger) {}
    std::optional<CodeObject> swap_code_object(const RecordedCodeObject& source, const std::unique_ptr<Buffer>& debug_buffer);
    void prepare_symbol_swap(const RecordedCodeObject& source, CodeObjectLoader& co_loader, hsa_agent_t agent);
    std::optional<hsa_executable_symbol_t> swap_symbol(hsa_executable_symbol_t sym);
};
}; // namespace agent
