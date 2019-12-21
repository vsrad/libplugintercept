#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <mutex>
#include <sstream>

namespace agent
{
class Logger
{
public:
    virtual void info(const std::string& msg) = 0;
    virtual void error(const std::string& msg) = 0;
    virtual void warning(const std::string& msg) = 0;
};

class AgentLogger : public Logger
{
private:
    std::reference_wrapper<std::ostream> _out;
    std::ofstream _file_output;

public:
    AgentLogger(const std::string& path) : _out(std::ref(std::cout)), _file_output()
    {
        if (path != "-")
        {
            _file_output.open(path, std::ios::out | std::ios::app);
            _out = std::ref<std::ostream>(_file_output);
        }
    }

    void info(const std::string& msg) override { _out.get() << "[INFO] " << msg << std::endl; }
    void error(const std::string& msg) override { _out.get() << "[ERROR] " << msg << std::endl; }
    void warning(const std::string& msg) override { _out.get() << "[WARNING] " << msg << std::endl; }
};

class CodeObjectLogger : public Logger
{
private:
    std::reference_wrapper<std::ostream> _out;
    std::ofstream _file_output;
    std::mutex _lock;
    std::ostringstream _msg_stream;

    void prepare_msg(const agent::CodeObject& co, const std::string& msg)
    {
        _msg_stream.str("");
        _msg_stream << "crc: " << co.CRC() << " " << msg;
    }

public:
    CodeObjectLogger(const std::string& path) : _out(std::ref(std::cout)), _file_output()
    {
        if (path != "-")
        {
            _file_output.open(path, std::ios::out | std::ios::app);
            _out = std::ref<std::ostream>(_file_output);
        }
    }

    void info(const std::string& msg) override { _out.get() << "[CO INFO] " << msg << std::endl; }
    void error(const std::string& msg) override { _out.get() << "[CO ERROR] " << msg << std::endl; }
    void warning(const std::string& msg) override { _out.get() << "[CO WARNING] " << msg << std::endl; }

    void info(const agent::CodeObject& co, const std::string& msg)
    {
        std::lock_guard lock(_lock);
        prepare_msg(co, msg);

        info(_msg_stream.str());
    }
    
    void error(const agent::CodeObject& co, const std::string& msg)
    {
        std::lock_guard lock(_lock);
        prepare_msg(co, msg);

        error(_msg_stream.str());
    }
    
    void warning(const agent::CodeObject& co, const std::string& msg)
    {
        std::lock_guard lock(_lock);
        prepare_msg(co, msg);

        info(_msg_stream.str());
    }
};
} // namespace agent
