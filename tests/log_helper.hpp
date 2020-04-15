#pragma once

#include "../src/logger/logger.hpp"

struct TestLogger : agent::AgentLogger
{
    TestLogger() : agent::AgentLogger("-") {}
    std::vector<std::string> infos;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    virtual void info(const std::string& msg) override { infos.push_back(msg); }
    virtual void error(const std::string& msg) override { errors.push_back(msg); }
    virtual void warning(const std::string& msg) override { warnings.push_back(msg); }
};
