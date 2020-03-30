#include "code_object_swapper.hpp"
#include "external_command.hpp"
#include <sstream>

using namespace agent;

SwapResult CodeObjectSwapper::try_swap(hsa_agent_t agent, const RecordedCodeObject& source, std::map<std::string, std::string> env)
{
    auto swap = std::find_if(_swaps.begin(), _swaps.end(), [&source](auto const& s) { return s.condition.matches(source); });
    if (swap == _swaps.end())
        return {};

    _logger.info("Substituting code object " + source.info() + " with " + swap->replacement_path);

    if (!swap->external_command.empty())
        if (!run_external_command(swap->external_command, env))
            return {};

    SwapResult result = {};
    if (auto replacement_co = CodeObject::try_read_from_file(swap->replacement_path.c_str()))
    {
        result.replacement_co.emplace(std::move(*replacement_co));
    }
    else
    {
        _logger.error("Unable to load replacement code object from " + swap->replacement_path);
        return {};
    }
    if (!swap->trap_handler_path.empty())
    {
        if (auto trap_handler_co = CodeObject::try_read_from_file(swap->trap_handler_path.c_str()))
            result.trap_handler_co.emplace(std::move(*trap_handler_co));
        else
            _logger.warning("Unable to load trap handler code object from " + swap->trap_handler_path);
    }

    return result;
}

bool CodeObjectSwapper::run_external_command(const std::string& cmd, std::map<std::string, std::string> env)
{
    _logger.info("Executing `" + cmd + "`");

    ExternalCommand runner(cmd);
    int retcode = runner.execute(env);
    if (retcode == 0)
    {
        _logger.info("The command has finished successfully");
        return true;
    }

    std::string error_log;
    error_log.append("The command `").append(cmd).append("` has exited with code ").append(std::to_string(retcode));
    if (auto stdout{runner.read_stdout()}; !stdout.empty())
        error_log.append("\n=== Stdout:\n").append(stdout);
    if (auto stderr{runner.read_stderr()}; !stderr.empty())
        error_log.append("\n=== Stderr:\n").append(stderr);
    _logger.error(error_log);
    return false;
}

hsa_status_t find_substitute_symbol(hsa_executable_t exec, hsa_executable_symbol_t sym, void* data)
{
    auto [target, target_name] = *reinterpret_cast<std::pair<hsa_executable_symbol_t*, const std::string*>*>(data);

    uint32_t name_len;
    hsa_status_t status = hsa_executable_symbol_get_info(sym, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_len);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    std::string current_sym_name(name_len, '\0');
    status = hsa_executable_symbol_get_info(sym, HSA_EXECUTABLE_SYMBOL_INFO_NAME, current_sym_name.data());
    if (status == HSA_STATUS_SUCCESS && current_sym_name == *target_name)
        *target = sym;

    return status;
}

void CodeObjectSwapper::prepare_symbol_substitutes(hsa_agent_t agent, const RecordedCodeObject& source, std::map<std::string, std::string> env)
{
    for (const auto& sub : _symbol_subs)
    {
        if (!sub.condition.matches(source))
            continue;
        if (auto sym{source.symbols().find(sub.source_name)}; sym != source.symbols().end())
        {
            if (!sub.external_command.empty())
                if (!run_external_command(sub.external_command, env))
                    continue;
            if (auto replacement_co = CodeObject::try_read_from_file(sub.replacement_path.c_str()))
            {
                const char* error_callsite;
                hsa_executable_t replacement_exec;
                hsa_executable_symbol_t replacement_sym{0};
                hsa_status_t status = _co_loader.create_executable(*replacement_co, agent, &replacement_exec, &error_callsite);
                if (status != HSA_STATUS_SUCCESS)
                {
                    _logger.hsa_error("Unable to load replacement code object from " + sub.replacement_path, status, error_callsite);
                    continue;
                }

                auto lookup_data = std::make_pair<hsa_executable_symbol_t*, const std::string*>(&replacement_sym, &sub.replacement_name);
                status = hsa_executable_iterate_symbols(replacement_exec, find_substitute_symbol, &lookup_data);
                if (status != HSA_STATUS_SUCCESS || replacement_sym.handle == 0)
                {
                    _logger.hsa_error("Unable to find " + sub.replacement_name + " in the replacement code object", status, "hsa_executable_iterate_symbols");
                    continue;
                }

                _evaluated_symbol_subs[sym->second.handle] = replacement_sym;
            }
            else
            {
                _logger.error("Unable to load replacement code object from " + sub.replacement_path);
            }
        }
    }
}

std::optional<hsa_executable_symbol_t> CodeObjectSwapper::substitute_symbol(hsa_executable_symbol_t sym)
{
    if (auto it{_evaluated_symbol_subs.find(sym.handle)}; it != _evaluated_symbol_subs.end())
        return {it->second};
    return {};
}
