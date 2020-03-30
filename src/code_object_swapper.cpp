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
        _logger.error("Unable to load the replacement code object from " + swap->replacement_path);
        return {};
    }
    if (!swap->trap_handler_path.empty())
    {
        if (auto trap_handler_co = CodeObject::try_read_from_file(swap->trap_handler_path.c_str()))
            result.trap_handler_co.emplace(std::move(*trap_handler_co));
        else
            _logger.warning("Unable to load trap handler code object from " + swap->trap_handler_path);
    }

    if (swap->symbol_swaps.empty())
        return result;

    const char* error_callsite;
    hsa_executable_t executable;
    hsa_status_t status = _co_loader.create_executable(*result.replacement_co, agent, &executable, &error_callsite);
    if (status == HSA_STATUS_SUCCESS)
    {
        auto mapper_data = std::make_tuple<CodeObjectSwapper*, const RecordedCodeObject*, const CodeObjectSwap*>(this, &source, &*swap);
        status = hsa_executable_iterate_symbols(executable, map_swapped_symbols, &mapper_data);
        error_callsite = "hsa_executable_iterate_symbols";
    }
    if (status != HSA_STATUS_SUCCESS)
        _logger.hsa_error("Unable to load the replacement code object from " + swap->replacement_path, status, error_callsite);

    return {};
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

hsa_status_t CodeObjectSwapper::map_swapped_symbols(hsa_executable_t exec, hsa_executable_symbol_t sym, void* data)
{
    auto [this_, source, swap] = *reinterpret_cast<std::tuple<CodeObjectSwapper*, const RecordedCodeObject*, const CodeObjectSwap*>*>(data);

    uint32_t name_len;
    hsa_status_t status = hsa_executable_symbol_get_info(sym, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_len);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    std::string current_sym_name(name_len, '\0');
    status = hsa_executable_symbol_get_info(sym, HSA_EXECUTABLE_SYMBOL_INFO_NAME, current_sym_name.data());
    if (status != HSA_STATUS_SUCCESS)
        return status;

    auto& src_symbols = source->symbols();

    for (auto& [src_sym_name, repl_sym_name] : swap->symbol_swaps)
    {
        if (current_sym_name != repl_sym_name)
            continue;
        if (auto src_sym{src_symbols.find(src_sym_name)}; src_sym != src_symbols.end())
            this_->_swapped_symbols[src_sym->second.handle] = sym;
        else
            this_->_logger.warning(
                "Symbol " + src_sym_name + " not found in code object, will not be replaced with " + repl_sym_name);
    }

    return HSA_STATUS_SUCCESS;
}

std::optional<hsa_executable_symbol_t> CodeObjectSwapper::swap_symbol(hsa_executable_symbol_t sym)
{
    if (auto it{_swapped_symbols.find(sym.handle)}; it != _swapped_symbols.end())
        return {it->second};
    return {};
}
