#pragma once

#include "code_object_swap.hpp"
#include <memory>
#include <vector>

namespace agent
{
class Config
{
private:
    uint64_t _debug_buffer_size;
    std::string _debug_buffer_dump_file;
    std::string _code_object_log_file;
    std::string _code_object_dump_dir;
    std::shared_ptr<std::vector<CodeObjectSwap>> _code_object_swaps;

public:
    Config();
    uint64_t debug_buffer_size() const { return _debug_buffer_size; }
    const std::string& debug_buffer_dump_file() const { return _debug_buffer_dump_file; }
    const std::string& code_object_log_file() const { return _code_object_log_file; }
    const std::string& code_object_dump_dir() const { return _code_object_dump_dir; }
    std::shared_ptr<std::vector<CodeObjectSwap>> code_object_swaps() const { return _code_object_swaps; }
};
} // namespace agent
