#pragma once

#include "logger/logger.hpp"
#include <iostream>
#include <vector>

namespace agent
{
typedef std::vector<std::pair<std::string, std::string>> ext_environment_t;

class ExternalCommand
{
private:
    std::FILE* _stdout_log;
    std::FILE* _stderr_log;

public:
    ExternalCommand() : _stdout_log(tmpfile()), _stderr_log(tmpfile()) {}
    ~ExternalCommand()
    {
        if (_stdout_log)
            fclose(_stdout_log);
        if (_stderr_log)
            fclose(_stderr_log);
    }
    int execute(const std::string& cmd, const ext_environment_t& env);
    std::string read_stdout();
    std::string read_stderr();

    static bool run_logged(const std::string& cmd, const ext_environment_t& env, AgentLogger& logger);
};
} // namespace agent
