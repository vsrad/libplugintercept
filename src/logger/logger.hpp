#pragma once

#include "../code_object.hpp"
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

class AgentLogger : public Logger<agent_info, agent_error, agent_warning>
{
public:
    AgentLogger(const std::string& path) : Logger(path) {}
    void hsa_error(std::string msg, hsa_status_t status, const char* error_callsite);
};

extern const char co_info[], co_error[], co_warning[];

class CodeObjectLogger : public Logger<co_info, co_error, co_warning>
{
private:
    std::string co_msg(const agent::CodeObject& co, const std::string& msg);

public:
    CodeObjectLogger(const std::string& path) : Logger(path) {}

    using Logger::info, Logger::error, Logger::warning;

    void info(const agent::CodeObject& co, const std::string& msg) { info(co_msg(co, msg)); }
    void error(const agent::CodeObject& co, const std::string& msg) { error(co_msg(co, msg)); }
    void warning(const agent::CodeObject& co, const std::string& msg) { warning(co_msg(co, msg)); }
};
} // namespace agent
