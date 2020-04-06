#include "external_command.hpp"
#include <sstream>

using namespace agent;

void ExternalCommand::run_init_command(const config::InitCommand& cmd, const ext_environment_t& env, AgentLogger& logger)
{
    if (cmd.command.empty())
        return;

    logger.info("Executing init command `" + cmd.command + "`");

    ExternalCommand runner;
    int retcode = runner.execute(cmd.command, env);
    if (cmd.expect_retcode && retcode != *cmd.expect_retcode)
    {
        std::string error_log;
        error_log.append("The command `").append(cmd.command).append("` has exited with code ").append(std::to_string(retcode));
        error_log.append(" (expected ").append(std::to_string(*cmd.expect_retcode)).append(")");
        if (auto stdout{runner.read_stdout()}; !stdout.empty())
            error_log.append("\n=== Stdout:\n").append(stdout);
        if (auto stderr{runner.read_stderr()}; !stderr.empty())
            error_log.append("\n=== Stderr:\n").append(stderr);
        logger.error(error_log);

        throw std::runtime_error("libplugintercept: init command has failed with code " + std::to_string(retcode));
    }
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
