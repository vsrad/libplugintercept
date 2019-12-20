#include "external_command.hpp"
#include <sstream>

using namespace agent;

std::string read_stream(std::FILE* stream)
{
    fseek(stream, 0, SEEK_END);
    size_t out_size = ftell(stream);
    rewind(stream);

    std::string out;
    out.resize(out_size);
    fread(&out[0], 1, out_size, stream);
    return out;
}

int ExternalCommand::execute(const std::map<std::string, std::string>& env)
{
    std::ostringstream cmd;
    cmd << "( ";
    for (auto it = env.begin(); it != env.end(); ++it)
        cmd << "export " << it->first << "=" << "\"" << it->second << "\";";
    cmd  << _command << " ) >&" << fileno(_stdout_log) << " 2>&" << fileno(_stderr_log);

    const std::string& cmd_string = cmd.str();
    return system(cmd_string.c_str());
}

std::string ExternalCommand::read_stdout()
{
    return read_stream(_stdout_log);
}

std::string ExternalCommand::read_stderr()
{
    return read_stream(_stderr_log);
}
