#pragma once

#include <iostream>

namespace agent
{
class ExternalCommand
{
private:
    std::string _command;
    std::FILE* _stdout_log;
    std::FILE* _stderr_log;

public:
    ExternalCommand(const std::string& cmd) : _command("{ " + cmd + " ;}"),
                                              _stdout_log(tmpfile()),
                                              _stderr_log(tmpfile()) {}
    ~ExternalCommand()
    {
        if (_stdout_log)
            fclose(_stdout_log);
        if (_stderr_log)
            fclose(_stderr_log);
    }
    int execute();
    std::string read_stdout();
    std::string read_stderr();
};
} // namespace agent
