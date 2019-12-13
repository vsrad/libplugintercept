#pragma once

#include "../CodeObject.hpp"
#include <mutex>
#include <fstream>

namespace agent
{
namespace logger
{

enum LogType
{
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
};

class CodeObjectLogger
{
private:
    std::mutex _lock;
    std::ofstream _fstream;

public:
    CodeObjectLogger();

    void Log(agent::CodeObject& code_object, LogType type, const char* message);
};

} // namespace logger
} // namespace agent