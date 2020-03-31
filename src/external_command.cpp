#include "external_command.hpp"
#include <sstream>

using namespace agent;

bool ExternalCommand::run_logged(const std::string& cmd, const ext_environment_t& env, AgentLogger& logger)
{
    logger.info("Executing `" + cmd + "`");

    ExternalCommand runner;
    int retcode = runner.execute(cmd, env);
    if (retcode == 0)
    {
        logger.info("The command has finished successfully");
        return true;
    }

    std::string error_log;
    error_log.append("The command `").append(cmd).append("` has exited with code ").append(std::to_string(retcode));
    if (auto stdout{runner.read_stdout()}; !stdout.empty())
        error_log.append("\n=== Stdout:\n").append(stdout);
    if (auto stderr{runner.read_stderr()}; !stderr.empty())
        error_log.append("\n=== Stderr:\n").append(stderr);
    logger.error(error_log);
    return false;
}

int ExternalCommand::execute(const std::string& cmd, const ext_environment_t& env)
{
    std::ostringstream shell_cmd;
    shell_cmd << "( ";
    for (const auto& [k, v] : env)
        shell_cmd << "export " << k << "="
                  << "\"" << v << "\";";
    shell_cmd << cmd << " )";
    shell_cmd << " >&" << fileno(_stdout_log) << " 2>&" << fileno(_stderr_log);

    const auto& cmd_string = shell_cmd.str();
    return system(cmd_string.c_str());
}

std::string read_stream(std::FILE* stream)
{
    fseek(stream, 0, SEEK_END);
    size_t out_size = ftell(stream);
    rewind(stream);

    std::string out;
    out.resize(out_size);
    fread(out.data(), 1, out_size, stream);
    return out;
}

std::string ExternalCommand::read_stdout()
{
    return read_stream(_stdout_log);
}

std::string ExternalCommand::read_stderr()
{
    return read_stream(_stderr_log);
}
