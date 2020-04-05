#include "config.hpp"

#include <cpptoml.h>
#include <sstream>

using namespace agent::config;

template <typename T>
T get_required(const cpptoml::table& table, const std::string& key, const std::string& description = {})
{
    if (auto value = table.get_qualified_as<T>(key))
        return *value;
    throw std::runtime_error("unable to read " + (description.empty() ? key : description));
}

CodeObjectSubstitute get_co_substitute(const cpptoml::table& config)
{
    CodeObjectSubstitute sub;
    sub.replacement_path = get_required<std::string>(config, "co-path", "code-object-replace.co-path (path to the replacement code object)");
    if (auto crc = config.get_qualified_as<crc32_t>("condition.co-crc"))
        sub.condition_crc = *crc;
    if (auto load_id = config.get_qualified_as<load_call_id_t>("condition.co-load-id"))
        sub.condition_load_id = *load_id;
    if (!sub.condition_crc && !sub.condition_load_id)
        throw std::runtime_error("unable to read code-object-replace.condition (no valid predicates specified)");
    return sub;
}

CodeObjectSymbolSubstitute get_symbol_substitute(const cpptoml::table& config)
{
    CodeObjectSymbolSubstitute sub;
    sub.replacement_name = get_required<std::string>(config, "replace-name", "kernel-replace.replace-name (name of the replacement kernel)");
    sub.replacement_path = get_required<std::string>(config, "co-path", "kernel-replace.co-path (path to the code object containing the replacement kernel)");
    if (auto crc = config.get_qualified_as<crc32_t>("condition.co-crc"))
        sub.condition_crc = *crc;
    if (auto load_id = config.get_qualified_as<load_call_id_t>("condition.co-load-id"))
        sub.condition_load_id = *load_id;
    if (auto kernel_name = config.get_qualified_as<std::string>("condition.kernel-name"))
        sub.condition_name = *kernel_name;
    if (auto get_info_id = config.get_qualified_as<get_info_call_id_t>("condition.kernel-get-id"))
        sub.condition_get_info_id = *get_info_id;
    if (!sub.condition_crc && !sub.condition_load_id && !sub.condition_name && !sub.condition_get_info_id)
        throw std::runtime_error("unable to read kernel-replace.condition (no valid predicates specified)");
    return sub;
}

Buffer get_buffer(const cpptoml::table& buffer_config)
{
    return {.size = get_required<uint64_t>(buffer_config, "size", "buffer.size"),
            .dump_path = buffer_config.get_as<std::string>("dump-path").value_or(""),
            .addr_env_name = buffer_config.get_as<std::string>("addr-env-name").value_or(""),
            .size_env_name = buffer_config.get_as<std::string>("size-env-name").value_or("")};
}

InitCommand get_init_command(const cpptoml::table& config)
{
    InitCommand c{.command = get_required<std::string>(config, "exec", "init-command.exec (shell command)")};
    if (auto retcode = config.get_as<int32_t>("required-return-code"))
        c.expect_retcode = *retcode;
    return c;
}

TrapHandler get_trap_handler(const cpptoml::table& config)
{
    return {.code_object_path = get_required<std::string>(config, "co-path", "trap-handler.co-path (path to the code object containing the trap handler kernel)"),
            .symbol_name = get_required<std::string>(config, "handler-name", "trap-handler.handler-name (name of the trap handler kernel)")};
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

            // Optional
            _code_object_dump_dir = config->get_qualified_as<std::string>("code-object-dump.directory").value_or("");

            if (auto buffer_configs = config->get_table_array("buffer"))
                for (const auto& buffer_config : *buffer_configs)
                    _buffers.push_back(get_buffer(*buffer_config));
            if (auto init_command_config = config->get_table("init-command"))
                _init_command = get_init_command(*init_command_config);
            if (auto trap_handler_config = config->get_table("trap-handler"))
                _trap_handler = get_trap_handler(*trap_handler_config);
            if (auto sub_configs = config->get_table_array("code-object-replace"))
                for (const auto& sub_config : *sub_configs)
                    _code_object_subs.push_back(get_co_substitute(*sub_config));
            if (auto sub_configs = config->get_table_array("kernel-replace"))
                for (const auto& sub_config : *sub_configs)
                    _symbol_subs.push_back(get_symbol_substitute(*sub_config));
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
