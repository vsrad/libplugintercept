#pragma once

#include "CodeObject.hpp"
#include "logger/logger.hpp"
#include <atomic>
#include <memory>
#include <variant>
#include <vector>

typedef uint64_t call_count_t;

namespace agent
{
struct CodeObjectSwapRequest
{
    std::variant<call_count_t, crc32_t> condition;
    std::string replacement_path;
    std::string external_command;
};

class CodeObjectSwapper
{
private:
    std::vector<CodeObjectSwapRequest> _swap_requests;
    std::shared_ptr<Logger> _logger;
    std::atomic<uint64_t> _call_counter;

public:
    CodeObjectSwapper(
        std::vector<CodeObjectSwapRequest> swap_requests,
        std::shared_ptr<Logger> logger) : _swap_requests(swap_requests),
                                          _logger(logger),
                                          _call_counter(0) {}
    std::optional<CodeObject> get_swapped_code_object(const CodeObject& source);
};
}; // namespace agent
