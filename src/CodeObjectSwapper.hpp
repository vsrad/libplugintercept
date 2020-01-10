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

public:
    CodeObjectSwapper(std::shared_ptr<std::vector<CodeObjectSwap>> swaps, std::shared_ptr<AgentLogger> logger)
        : _swaps(swaps), _logger(logger), _call_counter(0) {}
    std::optional<CodeObject> get_swapped_code_object(const CodeObject& source, const std::unique_ptr<Buffer>& debug_buffer);
};
}; // namespace agent
