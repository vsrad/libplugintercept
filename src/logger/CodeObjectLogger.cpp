#include "CodeObjectLogger.hpp"

#include "../CodeObject.hpp"
#include <iostream>
#include <sstream>

namespace agent
{
namespace logger
{

bool _log_to_file;

CodeObjectLogger::CodeObjectLogger()
{
    auto log_path = getenv("ASM_DBG_CO_LOG_PATH");

    if (!log_path)
        throw std::invalid_argument("Error: ASM_DBG_CO_LOG_PATH environment variable is not set");

    auto log_path_str = std::string(log_path);
    if (log_path_str.compare("-") == 0)
        _log_to_file = false;
    else
    {
        _fstream = std::ofstream(log_path, std::ios::out | std::ios::app);

        if (!_fstream.is_open())
            throw std::ios_base::failure("Error: cannot open code object log file to write logs");

        _log_to_file = true;
    }
}

void CodeObjectLogger::Log(agent::CodeObject& code_object, LogType type, const char* message)
{
    std::lock_guard lock(_lock);

    std::ostringstream message_stream;
    message_stream << "-- ";

    switch (type)
    {
    case INFO:
        message_stream << "INFO";
        break;
    case WARNING:
        message_stream << "WARNING";
        break;
    case ERROR:
        message_stream << "ERROR";
        break;
    default:
        break;
    }

    message_stream << " crc: "
                   << code_object.CRC() << " "
                   << message
                   << std::endl;

    if (_log_to_file)
        _fstream << message_stream.str();
    else
        std::cout << message_stream.str();
}

} // namespace logger
} // namespace agent