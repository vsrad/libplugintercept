#pragma once

#include <ostream>
#include <string>

namespace agent
{
struct TrapHandlerConfig
{
    std::string code_object_path;
    std::string symbol_name;
    std::string external_command;

    bool operator!=(const TrapHandlerConfig& rhs) const { return !operator==(rhs); }
    bool operator==(const TrapHandlerConfig& rhs) const
    {
        return code_object_path == rhs.code_object_path && symbol_name == rhs.symbol_name && external_command == rhs.external_command;
    }

    friend std::ostream& operator<<(std::ostream& os, const TrapHandlerConfig& th)
    {
        os << "{ code_object_path = " << th.code_object_path << ", symbol_name = " << th.symbol_name;
        if (!th.external_command.empty())
            os << ", external_command = " << th.external_command;
        os << " }";
        return os;
    }
};
} // namespace agent
