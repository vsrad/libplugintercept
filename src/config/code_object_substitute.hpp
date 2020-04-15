#pragma once

#include "../code_object.hpp"
#include <ostream>
#include <string>
#include <vector>

namespace agent::config
{
struct CodeObjectSubstitute
{
    std::optional<crc32_t> condition_crc;
    std::optional<load_call_id_t> condition_load_id;
    std::string replacement_path;

    bool operator!=(const CodeObjectSubstitute& rhs) const { return !operator==(rhs); }
    bool operator==(const CodeObjectSubstitute& rhs) const
    {
        return condition_crc == rhs.condition_crc && condition_load_id == rhs.condition_load_id && replacement_path == rhs.replacement_path;
    }
    friend std::ostream& operator<<(std::ostream& os, const CodeObjectSubstitute& sub)
    {
        os << "{";
        if (sub.condition_crc)
            os << " condition_crc = " << *sub.condition_crc << ",";
        if (sub.condition_load_id)
            os << " condition_load_id = " << *sub.condition_load_id << ",";
        os << " replacement_path = " << sub.replacement_path << " }";
        return os;
    }
};

struct CodeObjectSymbolSubstitute
{
    std::optional<crc32_t> condition_crc;
    std::optional<load_call_id_t> condition_load_id;
    std::optional<get_info_call_id_t> condition_get_info_id;
    std::optional<std::string> condition_name;
    std::string replacement_name;
    std::string replacement_path;

    bool operator!=(const CodeObjectSymbolSubstitute& rhs) const { return !operator==(rhs); }
    bool operator==(const CodeObjectSymbolSubstitute& rhs) const
    {
        return condition_crc == rhs.condition_crc &&
               condition_load_id == rhs.condition_load_id &&
               condition_get_info_id == rhs.condition_get_info_id &&
               condition_name == rhs.condition_name &&
               replacement_name == rhs.replacement_name &&
               replacement_path == rhs.replacement_path;
    }
    friend std::ostream& operator<<(std::ostream& os, const CodeObjectSymbolSubstitute& sub)
    {
        os << "{";
        if (sub.condition_crc)
            os << " condition_crc = " << *sub.condition_crc << ",";
        if (sub.condition_load_id)
            os << " condition_load_id = " << *sub.condition_load_id << ",";
        if (sub.condition_get_info_id)
            os << " condition_get_info_id = " << *sub.condition_get_info_id << ",";
        if (sub.condition_name)
            os << " condition_name = " << *sub.condition_name << ",";
        os << " replacement_name = " << sub.replacement_name << ",";
        os << " replacement_path = " << sub.replacement_path << " }";
        return os;
    }
};
} // namespace agent::config
