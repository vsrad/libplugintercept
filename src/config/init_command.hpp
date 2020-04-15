#pragma once

#include <optional>
#include <ostream>
#include <string>

namespace agent::config
{
struct InitCommand
{
    std::string command;
    std::optional<int32_t> expect_retcode;

    bool operator!=(const InitCommand& rhs) const { return !operator==(rhs); }
    bool operator==(const InitCommand& rhs) const
    {
        return command == rhs.command && expect_retcode == rhs.expect_retcode;
    }

    friend std::ostream& operator<<(std::ostream& os, const InitCommand& c)
    {
        os << "{ command = " << c.command;
        if (c.expect_retcode)
            os << ", expect_retcode = " << *c.expect_retcode;
        os << " }";
        return os;
    }
};
} // namespace agent::config
