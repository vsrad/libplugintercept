#pragma once

#include "../CodeObject.hpp"
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>

namespace agent
{
template <char const* info_marker, char const* warning_marker, char const* error_marker>
class Logger
{
private:
    std::reference_wrapper<std::ostream> _out;
    std::ofstream _file_output;
    std::mutex _lock;

public:
    Logger(const std::string& path) : _out(std::ref(std::cout)), _file_output()
    {
        if (path != "-")
        {
            _file_output.open(path, std::ios::out | std::ios::app);
            _out = std::ref<std::ostream>(_file_output);
        }
    }

    virtual void info(const std::string& msg)
    {
        std::lock_guard lock(_lock);
        _out.get() << info_marker << msg << std::endl;
    }

    virtual void error(const std::string& msg)
    {
        std::lock_guard lock(_lock);
        _out.get() << error_marker << msg << std::endl;
    }

    virtual void warning(const std::string& msg)
    {
        std::lock_guard lock(_lock);
        _out.get() << warning_marker << msg << std::endl;
    }
};

extern const char agent_info[], agent_error[], agent_warning[];

typedef Logger<agent_info, agent_error, agent_warning> AgentLogger;

extern const char co_info[], co_error[], co_warning[];

class CodeObjectLogger : public Logger<co_info, co_error, co_warning>
{
private:
    std::string co_msg(const agent::CodeObject& co, const std::string& msg)
    {
        return std::string("crc: ").append(std::to_string(co.CRC())).append(" ").append(msg);
    }

public:
    CodeObjectLogger(const std::string& path) : Logger(path) {}

    virtual void info(const std::string& msg) override { Logger::info(msg); }
    virtual void error(const std::string& msg) override { Logger::error(msg); }
    virtual void warning(const std::string& msg) override { Logger::warning(msg); }

    void info(const agent::CodeObject& co, const std::string& msg) { info(co_msg(co, msg)); }
    void error(const agent::CodeObject& co, const std::string& msg) { error(co_msg(co, msg)); }
    void warning(const agent::CodeObject& co, const std::string& msg) { warning(co_msg(co, msg)); }
};
} // namespace agent
