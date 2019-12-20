#include "CodeObjectSwapper.hpp"
#include "external_command.hpp"
#include <sstream>

using namespace agent;

std::optional<CodeObject> do_swap(const CodeObjectSwap& swap, const std::unique_ptr<Buffer>& debug_buffer, Logger& logger)
{
    if (!swap.external_command.empty())
    {
        logger.info("Executing `" + swap.external_command + "`");
        ExternalCommand cmd(swap.external_command);
        std::map<std::string, std::string> environment;

        if (debug_buffer)
        {
            environment["ASM_DBG_BUF_SIZE"] = std::to_string(debug_buffer->Size());
            environment["ASM_DBG_BUF_ADDR"] = std::to_string(reinterpret_cast<size_t>(debug_buffer->LocalPtr()));
        }
        else
        {
            logger.info("ASM_DBG_BUF_SIZE and ASM_DBG_BUF_ADDR are not set because the debug buffer has not been allocated yet.");
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
            logger.error(error_log.str());
            return {};
        }
        logger.info("The command has finished successfully");
    }

    auto new_co = CodeObject::try_read_from_file(swap.replacement_path.c_str());
    if (!new_co)
        logger.error("Unable to load the replacement code object from " + swap.replacement_path);
    return new_co;
}

std::optional<CodeObject> CodeObjectSwapper::get_swapped_code_object(const CodeObject& source, const std::unique_ptr<Buffer>& debug_buffer)
{
    call_count_t current_call = ++_call_counter;

    for (auto& swap : *_swaps)
        if (auto target_call_count = std::get_if<call_count_t>(&swap.condition))
        {
            if (*target_call_count == current_call)
            {
                _logger->info(
                    "Code object load #" + std::to_string(*target_call_count) + ": swapping for " + swap.replacement_path);
                return do_swap(swap, debug_buffer, *_logger);
            }
        }
        else if (auto target_crc = std::get_if<crc32_t>(&swap.condition))
        {
            if (*target_crc == source.CRC())
            {
                _logger->info(
                    "Code object load with CRC = " + std::to_string(*target_crc) + ": swapping for " + swap.replacement_path);
                return do_swap(swap, debug_buffer, *_logger);
            }
        }

    return {};
}
