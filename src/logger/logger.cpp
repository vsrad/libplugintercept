#include "logger.hpp"

using namespace agent;

namespace agent
{
const char agent_info[] = "[INFO] ", agent_error[] = "[ERROR] ", agent_warning[] = "[WARNING] ";
const char co_info[] = "[CO INFO] ", co_error[] = "[CO ERROR] ", co_warning[] = "[CO WARNING] ";
} // namespace agent

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
