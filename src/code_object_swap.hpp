#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <ostream>

typedef uint32_t crc32_t;
typedef uint64_t call_count_t;

namespace agent
{
struct CodeObjectSwap
{
    std::variant<call_count_t, crc32_t> condition;
    std::string replacement_path;
    std::string external_command;

    bool operator!=(const CodeObjectSwap& rhs) const { return !operator==(rhs); }
    bool operator==(const CodeObjectSwap& rhs) const
    {
        return condition == rhs.condition && replacement_path == rhs.replacement_path && external_command == rhs.external_command;
    }
    friend std::ostream& operator<<(std::ostream& os, const CodeObjectSwap& swap)
    {
        if (auto call_count = std::get_if<call_count_t>(&swap.condition))
            os << "call_count = " << *call_count;
        else if (auto crc = std::get_if<crc32_t>(&swap.condition))
            os << "crc = " << *crc;
        os << ", replacement_path = " << swap.replacement_path << ", external_command = " << swap.external_command;
        return os;
    }
};
} // namespace agent
