#include "CodeObjectLogger.hpp"

#include "../CodeObject.hpp"
#include <iostream>
#include <sstream>

namespace agent
{
namespace logger
{

CodeObjectLogger::CodeObjectLogger() {}

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

    std::cout << message_stream.str() << std::endl;
}

} // namespace logger
} // namespace agent