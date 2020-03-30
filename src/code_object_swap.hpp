#pragma once

#include "code_object.hpp"
#include <ostream>
#include <string>
#include <vector>

namespace agent
{
struct CodeObjectMatchCondition
{
    std::optional<crc32_t> crc;
    std::optional<load_call_id_t> load_call_id;

    bool matches(const RecordedCodeObject& co) const
    {
        if (crc && *crc != co.crc())
            return false;
        if (load_call_id && *load_call_id != co.load_call_id())
            return false;
        return true;
    }

    bool operator!=(const CodeObjectMatchCondition& rhs) const { return !operator==(rhs); }
    bool operator==(const CodeObjectMatchCondition& rhs) const
    {
        return crc == rhs.crc && load_call_id == rhs.load_call_id;
    }

    friend std::ostream& operator<<(std::ostream& os, const CodeObjectMatchCondition& cond)
    {
        os << "{";
        if (cond.crc)
            os << " crc = " << *cond.crc;
        if (cond.crc && cond.load_call_id)
            os << ",";
        if (cond.load_call_id)
            os << " load_call_id = " << *cond.load_call_id;
        os << " }";
        return os;
    }
};

struct CodeObjectSwap
{
    CodeObjectMatchCondition condition;
    std::string replacement_path;
    std::string trap_handler_path;
    std::string external_command;
    std::vector<std::pair<std::string, std::string>> symbol_swaps;

    bool operator!=(const CodeObjectSwap& rhs) const { return !operator==(rhs); }
    bool operator==(const CodeObjectSwap& rhs) const
    {
        return condition == rhs.condition &&
               replacement_path == rhs.replacement_path &&
               trap_handler_path == rhs.trap_handler_path &&
               external_command == rhs.external_command &&
               symbol_swaps == rhs.symbol_swaps;
    }
    friend std::ostream& operator<<(std::ostream& os, const CodeObjectSwap& swap)
    {
        os << "{ condition = " << swap.condition;
        os << ", replacement_path = " << swap.replacement_path;

        if (!swap.trap_handler_path.empty())
            os << ", trap_handler_path = " << swap.trap_handler_path;

        if (!swap.external_command.empty())
            os << ", external_command = " << swap.external_command;

        bool first_symbol = true;
        for (const auto& sym : swap.symbol_swaps)
        {
            os << (first_symbol ? ", symbols = [(" : ", (") << sym.first << ", " << sym.second << ")";
            first_symbol = false;
        }
        if (!first_symbol)
            os << "]";

        os << " }";
        return os;
    }
};
} // namespace agent
