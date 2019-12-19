#pragma once

#include <string>

namespace agent
{
class Config
{
private:
    uint64_t _debug_buffer_size;
    std::string _debug_buffer_dump_file;
    std::string _code_object_dump_dir;

public:
    Config(const std::string& config_file);
    uint64_t debug_buffer_size() const { return _debug_buffer_size; }
    const std::string& debug_buffer_dump_file() const { return _debug_buffer_dump_file; }
    const std::string& code_object_dump_dir() const { return _code_object_dump_dir; }
};
} // namespace agent
