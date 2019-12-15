#include "external_command.hpp"

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

int ExternalCommand::execute()
{
    auto cmd = _command + " >&" + std::to_string(fileno(_stdout_log)) + " 2>&" + std::to_string(fileno(_stderr_log));
    return system(cmd.c_str());
}

std::string ExternalCommand::read_stdout()
{
    return read_stream(_stdout_log);
}

std::string ExternalCommand::read_stderr()
{
    return read_stream(_stderr_log);
}
