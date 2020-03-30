#pragma once

#include "buffer_allocation.hpp"
#include "code_object_swap.hpp"
#include "trap_handler_config.hpp"
#include <vector>

namespace agent
{
class Config
{
private:
    std::string _agent_log_file;
    std::string _code_object_log_file;
    std::string _code_object_dump_dir;
    std::vector<BufferAllocation> _buffer_allocations;
    std::vector<CodeObjectSwap> _code_object_swaps;
    std::vector<CodeObjectSymbolSubstitute> _code_object_symbol_subs;
    TrapHandlerConfig _trap_handler;

public:
    Config();
    const std::string& agent_log_file() const { return _agent_log_file; }
    const std::string& code_object_log_file() const { return _code_object_log_file; }
    const std::string& code_object_dump_dir() const { return _code_object_dump_dir; }
    const std::vector<BufferAllocation>& buffer_allocations() const { return _buffer_allocations; }
    const std::vector<CodeObjectSwap>& code_object_swaps() const { return _code_object_swaps; }
    const std::vector<CodeObjectSymbolSubstitute>& code_object_symbol_subs() const { return _code_object_symbol_subs; }
    const TrapHandlerConfig& trap_handler() const { return _trap_handler; };
};
} // namespace agent
