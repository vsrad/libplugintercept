#include "config.hpp"

#include <cpptoml.h>
#include <sstream>

using namespace agent;

template <typename T>
T get_required(const cpptoml::table& table, const std::string& key)
{
    if (auto value = table.get_qualified_as<T>(key))
        return *value;
    throw std::runtime_error("missing or invalid value for " + key);
}

CodeObjectMatchCondition get_condition(const cpptoml::table& match)
{
    CodeObjectMatchCondition cond;
    if (auto crc = match.get_qualified_as<crc32_t>("crc"))
        cond.crc = {*crc};
    if (auto load_call_id = match.get_qualified_as<load_call_id_t>("load-call-id"))
        cond.load_call_id = {*load_call_id};
    if (!cond.crc && !cond.load_call_id)
        throw std::runtime_error("missing or invalid code object match condition (specify crc or load-call-id or both)");
    return cond;
}

CodeObjectSwap get_co_swap(const cpptoml::table& swap_config)
{
    CodeObjectSwap swap;
    if (auto match = swap_config.get_table("match"))
        swap.condition = get_condition(*match);
    else
        throw std::runtime_error("missing match condition (code-object-swap.match)");

    if (auto path = swap_config.get_as<std::string>("load-file"))
        swap.replacement_path = *path;
    else
        throw std::runtime_error("missing replacement file path (code-object-swap.load-file)");

    swap.trap_handler_path = swap_config.get_as<std::string>("load-trap-handler-file").value_or("");
    swap.external_command = swap_config.get_as<std::string>("exec-before-load").value_or("");

    if (auto symbols = swap_config.get_table_array("symbol"))
        for (const auto& symbol : *symbols)
        {
            auto name = symbol->get_as<std::string>("name");
            auto replacement_name = symbol->get_as<std::string>("replacement-name");
            if (!name)
                throw std::runtime_error("code-object-swap.symbol is missing name");
            if (!replacement_name)
                throw std::runtime_error("code-object-swap.symbol is missing replacement-name");
            swap.symbol_swaps.push_back({*name, *replacement_name});
        }

    return swap;
}

BufferAllocation get_buffer_alloc(const cpptoml::table& alloc_config)
{
    BufferAllocation alloc;
    if (auto size = alloc_config.get_as<uint64_t>("size"))
        alloc.size = *size;
    else
        throw std::runtime_error("missing buffer-allocation.size");
    alloc.dump_path = alloc_config.get_as<std::string>("dump-path").value_or("");
    alloc.addr_env_name = alloc_config.get_as<std::string>("addr-env-name").value_or("");
    alloc.size_env_name = alloc_config.get_as<std::string>("size-env-name").value_or("");
    return alloc;
}

Config::Config()
{
    if (auto config_path_env = getenv("INTERCEPT_CONFIG"))
    {
        std::string config_path(config_path_env);
        try
        {
            auto config = cpptoml::parse_file(config_path);

            // Required
            _agent_log_file = get_required<std::string>(*config, "agent.log");
            _code_object_log_file = get_required<std::string>(*config, "code-object-dump.log");
            _code_object_dump_dir = get_required<std::string>(*config, "code-object-dump.directory");

            // Optional
            if (auto alloc_configs = config->get_table_array("buffer-allocation"))
                for (const auto& alloc_config : *alloc_configs)
                    _buffer_allocations.push_back(get_buffer_alloc(*alloc_config));
            if (auto swap_configs = config->get_table_array("code-object-swap"))
                for (const auto& swap_config : *swap_configs)
                    _code_object_swaps.push_back(get_co_swap(*swap_config));
        }
        catch (const std::runtime_error& e)
        {
            throw std::runtime_error("error when parsing configuration file " + config_path + ": " + e.what());
        }
    }
    else
    {
        throw std::runtime_error("INTERCEPT_CONFIG (path to the configuration file) is not set");
    }
}
