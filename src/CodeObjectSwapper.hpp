#pragma once

#include "CodeObject.hpp"
#include <atomic>
#include <optional>
#include <variant>
#include <vector>

typedef uint64_t call_count_t;

namespace agent
{
struct CodeObjectSwapRequest
{
    std::variant<call_count_t, crc32_t> condition;
    std::string replacement_path;
};

class CodeObjectSwapper
{
private:
    std::atomic<uint64_t> _call_counter;
    std::vector<CodeObjectSwapRequest> _swap_requests;

public:
    CodeObjectSwapper(std::vector<CodeObjectSwapRequest> swap_requests) : _call_counter(0),
                                                                          _swap_requests(swap_requests) {}
    std::optional<CodeObject> get_swapped_code_object(const CodeObject& source);
};
}; // namespace agent
