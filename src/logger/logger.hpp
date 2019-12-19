#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <ostream>
#include <string>

namespace agent
{
class Logger
{
public:
    virtual void info(const std::string& msg) = 0;
    virtual void error(const std::string& msg) = 0;
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
};
} // namespace agent
