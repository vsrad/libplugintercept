#include "config.hpp"

#include <cpptoml.h>
#include <sstream>

using namespace agent;

template <typename T>
T get_config(const std::string& config_path, const cpptoml::table& table, const std::string& key)
{
    if (auto value = table.get_qualified_as<T>(key))
        return *value;
    throw std::runtime_error("Error when parsing configuration file " + config_path + ": missing or invalid value for " + key);
}

CodeObjectSwap get_co_swap(const std::string& config_path, const cpptoml::table& swap_config)
{
    CodeObjectSwap swap;
    auto crc = swap_config.get_as<crc32_t>("when-crc");
    auto call_count = swap_config.get_as<call_count_t>("when-call-count");
    if (!crc && !call_count)
        throw std::runtime_error("Error when parsing configuration file " + config_path + ": missing code-object-swap condition (either when-crc or when-call-count)");
    if (crc && call_count)
        throw std::runtime_error("Error when parsing configuration file " + config_path + ": code-object-swap cannot have both when-crc and when-call-count as the condition");
    if (crc)
        swap.condition = {*crc};
    else if (call_count)
        swap.condition = {*call_count};

    swap.external_command = swap_config.get_as<std::string>("exec-before-load").value_or("");

    if (auto path = swap_config.get_as<std::string>("load-file"))
        swap.replacement_path = *path;
    else
        throw std::runtime_error("Error when parsing configuration file " + config_path + ": missing code-object-swap.load-file");

    if (auto symbols = swap_config.get_table_array("symbol"))
        for (const auto& symbol : *symbols)
        {
            auto name = symbol->get_as<std::string>("name");
            auto replacement_name = symbol->get_as<std::string>("replacement-name");
            if (!name)
                throw std::runtime_error("Error when parsing configuration file " + config_path + ": code-object-swap.symbol is missing name");
            if (!replacement_name)
                throw std::runtime_error("Error when parsing configuration file " + config_path + ": code-object-swap.symbol is missing replacement-name");
            swap.symbol_swaps.push_back({*name, *replacement_name});
        }

    return swap;
}

Config::Config()
{
    if (auto config_path_env = getenv("ASM_DBG_CONFIG"))
    {
        std::string config_path(config_path_env);
        auto config = cpptoml::parse_file(config_path);

        // Required
        _agent_log_file = get_config<std::string>(config_path, *config, "agent.log");
        _code_object_log_file = get_config<std::string>(config_path, *config, "code-object-dump.log");
        _code_object_dump_dir = get_config<std::string>(config_path, *config, "code-object-dump.directory");

        // Optional
        _debug_buffer_size = config->get_qualified_as<uint64_t>("debug-buffer.size").value_or(0);
        _debug_buffer_dump_file = config->get_qualified_as<std::string>("debug-buffer.dump-file").value_or("");

        _code_object_swaps = std::make_shared<std::vector<CodeObjectSwap>>();
        if (auto swap_configs = config->get_table_array("code-object-swap"))
            for (const auto& swap_config : *swap_configs)
                _code_object_swaps->push_back(get_co_swap(config_path, *swap_config));
    }
    else
    {
        throw std::runtime_error("ASM_DBG_CONFIG (path to the configuration file) is not set.");
    }
}
