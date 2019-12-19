#include "config.hpp"

#include <sstream>

using namespace agent;

Config::Config(const std::string& config_file)
{
    auto debug_path = getenv("ASM_DBG_PATH");
    auto write_path = getenv("ASM_DBG_WRITE_PATH");
    auto debug_size_env = getenv("ASM_DBG_BUF_SIZE");

    if (!debug_path || !debug_size_env || !write_path)
    {
        std::ostringstream exception_message;
        exception_message << "Error: environment variable is not set." << std::endl
                          << "-- ASM_DBG_PATH: " << _debug_buffer_dump_file << std::endl
                          << "-- ASM_DBG_BUF_SIZE: " << _debug_buffer_size << std::endl;

        throw std::invalid_argument(exception_message.str());
    }

    _debug_buffer_dump_file = debug_path;
    _debug_buffer_size = atoi(debug_size_env);
    _code_object_dump_dir = write_path;
}
