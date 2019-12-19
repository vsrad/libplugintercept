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

        _code_object_swaps = std::make_shared<std::vector<CodeObjectSwap>>();
        auto swap_configs = config->get_table_array("code-object-swap");
        for (const auto& swap_config : *swap_configs)
        {
            CodeObjectSwap swap;
            auto crc = swap_config->get_as<crc32_t>("when-crc");
            auto call_count = swap_config->get_as<call_count_t>("when-call-count");
            if (!crc && !call_count)
                throw std::runtime_error("Error when parsing configuration file " + config_path + ": missing code-object-swap condition (either when-crc or when-call-count)");
            if (crc && call_count)
                throw std::runtime_error("Error when parsing configuration file " + config_path + ": code-object-swap cannot have both when-crc and when-call-count as the condition");
            if (crc)
                swap.condition = {*crc};
            if (call_count)
                swap.condition = {*call_count};

            swap.external_command = swap_config->get_as<std::string>("exec-before-load").value_or("");

            auto path = swap_config->get_as<std::string>("load-file");
            if (path)
                swap.replacement_path = *path;
            else
                throw std::runtime_error("Error when parsing configuration file " + config_path + ": missing code-object-swap.load-file");

            _code_object_swaps->push_back(swap);
        }
    }
    else
    {
        throw std::runtime_error("ASM_DBG_CONFIG (path to the configuration file) is not set.");
    }
}
