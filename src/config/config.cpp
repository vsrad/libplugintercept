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

CodeObjectSubstitute get_co_substitute(const cpptoml::table& sub_config)
{
    return {};
    // return {
    //     .replacement_path = get_required<std::string>(sub_config, "load-file", "code-object-substitute.load-file (path to the replacement code object)"),
    //     .external_command = sub_config.get_as<std::string>("exec-before-load").value_or("")};
}

CodeObjectSymbolSubstitute get_symbol_substitute(const cpptoml::table& sub_config)
{
    return {};
    // CodeObjectSymbolSubstitute sub;
    // if (auto match = sub_config.get_table("match-code-object"))
    //     sub.condition = get_co_match_condition(match, "symbol-substitution.match-code-object");
    // sub.source_name = get_required<std::string>(sub_config, "match-name", "symbol-substitution.match-name (symbol name to replace)");
    // sub.replacement_name = get_required<std::string>(sub_config, "replace-with", "symbol-substitution.replace-with (symbol name in the replacement code object)");
    // sub.replacement_path = get_required<std::string>(sub_config, "load-file", "symbol-substitution.load-file (path to the replacement code object)");
    // sub.external_command = sub_config.get_as<std::string>("exec-before-load").value_or("");
    // return sub;
}

Buffer get_buffer(const cpptoml::table& buffer_config)
{
    return {
        .size = get_required<uint64_t>(buffer_config, "size", "buffer.size"),
        .dump_path = buffer_config.get_as<std::string>("dump-path").value_or(""),
        .addr_env_name = buffer_config.get_as<std::string>("addr-env-name").value_or(""),
        .size_env_name = buffer_config.get_as<std::string>("size-env-name").value_or("")};
}

InitCommand get_init_command(const cpptoml::table& config)
{
    InitCommand c;
    c.command = get_required<std::string>(config, "exec", "init-command.exec (shell command)");
    if (auto retcode = config.get_as<int32_t>("required-return-code"))
        c.expect_retcode = *retcode;
    return c;
}

TrapHandler get_trap_handler(const cpptoml::table& config)
{
    return {
        .code_object_path = get_required<std::string>(config, "load-file", "trap-handler.load-file (path to the code object containing the trap handler kernel)"),
        .symbol_name = get_required<std::string>(config, "handler-name", "trap-handler.handler-name (name of the trap handler kernel)"),
        .external_command = config.get_as<std::string>("exec-before-load").value_or("")};
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
            if (auto sub_configs = config->get_table_array("code-object-substitute"))
                for (const auto& sub_config : *sub_configs)
                    _code_object_subs.push_back(get_co_substitute(*sub_config));
            if (auto sub_configs = config->get_table_array("symbol-substitute"))
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
