#include "logger.hpp"
#include "../fs_utils.hpp"
#include <iostream>

using namespace agent;

Logger::Logger(const std::string& path, const char* info_marker, const char* warning_marker, const char* error_marker)
    : _info_marker(info_marker), _warning_marker(warning_marker), _error_marker(error_marker)
{
    if (path.empty()) // logging disabled
        return;

    _output_mutex = std::make_unique<std::mutex>();
    if (path == "-") // logging to stdout
        return;

    fs_utils::create_parent_directories(path.c_str());
    _file_output = std::make_unique<std::ofstream>(path, std::ios::out | std::ios::app);
    if (!*_file_output)
        std::cerr << "libplugintercept: unable to create a log file at " << path << std::endl;
}

void Logger::write(const char* marker, const std::string& msg)
{
    if (!_output_mutex) // logging disabled
        return;
    std::scoped_lock lock(*_output_mutex);
    if (_file_output)
        *_file_output << marker << msg << std::endl;
    else
        std::cout << marker << msg << std::endl;
}

void AgentLogger::hsa_error(std::string msg, hsa_status_t status, const char* error_callsite)
{
    const char* err;
    hsa_status_string(status, &err);
    error(msg.append(": ").append(error_callsite).append(" failed with ").append(err));
}

std::string CodeObjectLogger::co_msg(const RecordedCodeObject& co, const std::string& msg)
{
    return "CO " + co.info() + ": " + msg;
}
