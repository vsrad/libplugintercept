#include "config.hpp"

#include <cpptoml.h>
#include <sstream>

using namespace agent;

template <typename T>
T get_config(const std::string& config_path, const cpptoml::table& table, const std::string& key)
{
    auto value = table.get_qualified_as<T>(key);
    if (!value)
        throw std::runtime_error("Error when parsing configuration file " + config_path + ": missing or invalid value for " + key);
    return *value;
}

Config::Config()
{
    if (auto config_path_env = getenv("ASM_DBG_CONFIG"))
    {
        std::string config_path(config_path_env);

        auto config = cpptoml::parse_file(config_path);

        _debug_buffer_size = get_config<uint64_t>(config_path, *config, "debug-buffer.size");
        _debug_buffer_dump_file = get_config<std::string>(config_path, *config, "debug-buffer.dump-file");
        _code_object_log_file = get_config<std::string>(config_path, *config, "code-object-dump.log");
        _code_object_dump_dir = get_config<std::string>(config_path, *config, "code-object-dump.directory");
    }
    else
    {
        throw std::runtime_error("ASM_DBG_CONFIG (path to the configuration file) is not set.");
    }
}
