#include "code_object_swapper.hpp"
#include "external_command.hpp"
#include <sstream>

using namespace agent;

std::optional<CodeObject> CodeObjectSwapper::swap_code_object(
    const RecordedCodeObject& source,
    const std::unique_ptr<Buffer>& debug_buffer,
    hsa_agent_t agent)
{
    for (auto& swap : *_swaps)
        if (auto target_call_count = std::get_if<call_count_t>(&swap.condition))
        {
            if (*target_call_count == source.load_call_no())
            {
                _logger->info(
                    "Code object load #" + std::to_string(*target_call_count) + ": swapping for " + swap.replacement_path);
                return do_swap(swap, source, debug_buffer, agent);
            }
        }
        else if (auto target_crc = std::get_if<crc32_t>(&swap.condition))
        {
            if (*target_crc == source.crc())
            {
                _logger->info(
                    "Code object load with CRC = " + std::to_string(*target_crc) + ": swapping for " + swap.replacement_path);
                return do_swap(swap, source, debug_buffer, agent);
            }
        }

    return {};
}

std::optional<CodeObject> CodeObjectSwapper::do_swap(
    const CodeObjectSwap& swap,
    const RecordedCodeObject& source,
    const std::unique_ptr<Buffer>& debug_buffer,
    hsa_agent_t agent)
{
    if (!swap.external_command.empty())
    {
        _logger->info("Executing `" + swap.external_command + "`");
        ExternalCommand cmd(swap.external_command);
        std::map<std::string, std::string> environment;

        if (debug_buffer)
        {
            environment["ASM_DBG_BUF_SIZE"] = std::to_string(debug_buffer->Size());
            environment["ASM_DBG_BUF_ADDR"] = std::to_string(reinterpret_cast<size_t>(debug_buffer->LocalPtr()));
        }
        else
        {
            _logger->info("ASM_DBG_BUF_SIZE and ASM_DBG_BUF_ADDR are not set because the debug buffer has not been allocated yet.");
        }

        int retcode = cmd.execute(environment);
        if (retcode != 0)
        {
            std::ostringstream error_log;
            error_log << "The command `" << swap.external_command << "` has exited with code " << retcode;
            auto stdout = cmd.read_stdout();
            if (!stdout.empty())
                error_log << "\n=== Stdout:\n"
                          << stdout;
            auto stderr = cmd.read_stderr();
            if (!stderr.empty())
                error_log << "\n=== Stderr:\n"
                          << stderr;
            _logger->error(error_log.str());
            return {};
        }
        _logger->info("The command has finished successfully");
    }

    auto new_co = CodeObject::try_read_from_file(swap.replacement_path.c_str());
    if (!new_co)
        _logger->error("Unable to load the replacement code object from " + swap.replacement_path);

    if (!swap.symbol_swaps.empty())
    {
        const char* error_callsite;
        hsa_executable_t executable;
        hsa_status_t status = _co_loader.create_executable(*new_co, agent, &executable, &error_callsite);
        if (status == HSA_STATUS_SUCCESS)
        {
            auto mapper_data = std::make_tuple<CodeObjectSwapper*, const RecordedCodeObject*, const CodeObjectSwap*>(this, &source, &swap);
            status = hsa_executable_iterate_symbols(executable, map_swapped_symbols, &mapper_data);
            error_callsite = "hsa_executable_iterate_symbols";
        }
        if (status != HSA_STATUS_SUCCESS)
        {
            _logger->hsa_error("Unable to prepare replacement code object for CRC = " + std::to_string(source.crc()), status, error_callsite);
        }
        return {};
    }

    return new_co;
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
            this_->_logger->warning(
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
