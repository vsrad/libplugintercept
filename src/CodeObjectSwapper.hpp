#pragma once

#include "CodeObject.hpp"
#include "buffer.hpp"
#include "logger/logger.hpp"
#include <atomic>
#include <memory>
#include <vector>

namespace agent
{
class CodeObjectSwapper
{
private:
    std::shared_ptr<std::vector<CodeObjectSwap>> _swaps;
    std::shared_ptr<AgentLogger> _logger;
    std::atomic<uint64_t> _call_counter;
    std::unordered_map<std::shared_ptr<CodeObject>, std::pair<const CodeObjectSwap&, CodeObject>> _symbol_swaps;
    std::optional<CodeObject> do_swap(const CodeObjectSwap& swap, std::shared_ptr<CodeObject> source, const std::unique_ptr<Buffer>& debug_buffer);

public:
    CodeObjectSwapper(std::shared_ptr<std::vector<CodeObjectSwap>> swaps, std::shared_ptr<AgentLogger> logger)
        : _swaps(swaps), _logger(logger), _call_counter(0) {}
    std::optional<CodeObject> get_swapped_code_object(std::shared_ptr<CodeObject> source, const std::unique_ptr<Buffer>& debug_buffer);
};
}; // namespace agent
