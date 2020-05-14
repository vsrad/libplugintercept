#pragma once

#include "../code_object.hpp"
#include <fstream>
#include <hsakmt.h>
#include <memory>
#include <mutex>
#include <string>

namespace agent
{
class Logger
{
private:
    const char *_info_marker, *_warning_marker, *_error_marker;
    std::unique_ptr<std::mutex> _output_mutex;
    std::unique_ptr<std::ofstream> _file_output;

public:
    Logger(const std::string& path, const char* info_marker, const char* warning_marker, const char* error_marker);

    void write(const char* marker, const std::string& msg);

    virtual void info(const std::string& msg) { write(_info_marker, msg); }
    virtual void warning(const std::string& msg) { write(_warning_marker, msg); }
    virtual void error(const std::string& msg) { write(_error_marker, msg); }
};

class AgentLogger : public Logger
{
public:
    AgentLogger(const std::string& path)
        : Logger(path, "[INFO] ", "[WARNING] ", "[ERROR] ") {}

    void hsa_error(std::string msg, hsa_status_t status, const char* error_callsite);
    void hsa_kmt_error(std::string msg, HSAKMT_STATUS status, const char* error_callsite);
};

class CodeObjectLogger : public Logger
{
private:
    std::string co_msg(const RecordedCodeObject& co, const std::string& msg);

public:
    CodeObjectLogger(const std::string& path)
        : Logger(path, "[CO INFO] ", "[CO WARNING] ", "[CO ERROR] ") {}

    using Logger::info, Logger::error, Logger::warning;

    void info(const RecordedCodeObject& co, const std::string& msg) { info(co_msg(co, msg)); }
    void error(const RecordedCodeObject& co, const std::string& msg) { error(co_msg(co, msg)); }
    void warning(const RecordedCodeObject& co, const std::string& msg) { warning(co_msg(co, msg)); }
};
} // namespace agent
