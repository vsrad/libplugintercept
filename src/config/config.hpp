#pragma once

#include "buffer.hpp"
#include "code_object_substitute.hpp"
#include "init_command.hpp"
#include "trap_handler.hpp"
#include <vector>

namespace agent::config
{
class Config
{
private:
    std::string _agent_log_file;
    std::string _code_object_log_file;
    std::string _code_object_dump_dir;
    InitCommand _init_command;
    TrapHandler _trap_handler;
    std::vector<Buffer> _buffers;
    std::vector<CodeObjectSubstitute> _code_object_subs;
    std::vector<CodeObjectSymbolSubstitute> _symbol_subs;

public:
    Config();
    const std::string& agent_log_file() const { return _agent_log_file; }
    const std::string& code_object_log_file() const { return _code_object_log_file; }
    const std::string& code_object_dump_dir() const { return _code_object_dump_dir; }
    const InitCommand& init_command() const { return _init_command; };
    const TrapHandler& trap_handler() const { return _trap_handler; };
    const std::vector<Buffer>& buffers() const { return _buffers; }
    const std::vector<CodeObjectSubstitute>& code_object_subs() const { return _code_object_subs; }
    const std::vector<CodeObjectSymbolSubstitute>& symbol_subs() const { return _symbol_subs; }
};
} // namespace agent::config
