#pragma once

namespace agent
{
class Logger
{
public:
    virtual void info(const std::string& msg) = 0;
    virtual void error(const std::string& msg) = 0;
};
} // namespace agent
