#pragma once

#include "buffer_manager.hpp"
#include "code_object_loader.hpp"
#include "code_object_recorder.hpp"
#include "code_object_substitutor.hpp"
#include "config/config.hpp"
#include "logger/logger.hpp"
#include "trap_handler.hpp"
#include <memory>
#include <string>
#include <variant>

namespace agent
{
class DebugAgent
{
private:
    std::shared_ptr<config::Config> _config;
    std::shared_ptr<AgentLogger> _logger;
    std::unique_ptr<CodeObjectLoader> _co_loader;
    std::unique_ptr<CodeObjectRecorder> _co_recorder;
    std::unique_ptr<CodeObjectSubstitutor> _co_substitutor;
    std::unique_ptr<BufferManager> _buffer_manager;
    std::unique_ptr<TrapHandler> _trap_handler;

    std::mutex _agent_mutex;
    bool _first_executable_load{true};
    get_info_call_id_t _get_symbol_info_id{0};

public:
    DebugAgent(std::shared_ptr<config::Config> config,
               std::shared_ptr<AgentLogger> logger,
               std::shared_ptr<CodeObjectLogger> co_logger,
               std::unique_ptr<CodeObjectLoader> co_loader)
        : _config(config), _logger(logger), _co_loader(std::move(co_loader)),
          _co_recorder(std::make_unique<CodeObjectRecorder>(config->code_object_dump_dir(), co_logger)),
          _co_substitutor(std::make_unique<CodeObjectSubstitutor>(config->code_object_subs(), config->symbol_subs(), *_logger, *_co_loader)),
          _buffer_manager(std::make_unique<BufferManager>(config->buffers(), *_logger)),
          _trap_handler(std::make_unique<TrapHandler>(*_logger, *_co_loader, config->trap_handler())) {}

    void record_co_load(
        hsaco_t hsaco,
        const void* contents,
        size_t size,
        hsa_status_t load_status);

    hsa_status_t executable_load_co(
        hsaco_t hsaco,
        hsa_agent_t agent,
        hsa_executable_t executable,
        std::function<hsa_status_t(hsaco_t)> loader);

    hsa_status_t intercept_hsa_executable_symbol_get_info(
        decltype(hsa_executable_symbol_get_info)* intercepted_fn,
        hsa_executable_symbol_t executable_symbol,
        hsa_executable_symbol_info_t attribute,
        void* value);
};

} // namespace agent
