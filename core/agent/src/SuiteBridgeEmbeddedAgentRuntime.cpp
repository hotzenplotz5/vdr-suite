#include "SuiteBridgeEmbeddedAgentRuntime.h"

#include <algorithm>
#include <utility>

namespace vdrsuite::agent
{
namespace
{

constexpr std::size_t MaximumBackendIdLength = 128;

}

SuiteBridgeEmbeddedAgentRuntime::SuiteBridgeEmbeddedAgentRuntime(
    SuiteBridgeEmbeddedAgentConfig config)
    : config_(std::move(config))
{
    config_.backendId = boundedBackendId(config_.backendId);

    if (config_.enabled)
    {
        transport_ = std::make_unique<SuiteBridgeSvdrpTransport>(
            config_.transport);
    }

    initializeWorker();
}

SuiteBridgeEmbeddedAgentRuntime::SuiteBridgeEmbeddedAgentRuntime(
    SuiteBridgeEmbeddedAgentConfig config,
    std::unique_ptr<ISuiteBridgeLocalTransport> transport)
    : config_(std::move(config)),
      transport_(std::move(transport))
{
    config_.backendId = boundedBackendId(config_.backendId);
    initializeWorker();
}

SuiteBridgeEmbeddedAgentRuntime::~SuiteBridgeEmbeddedAgentRuntime()
{
    stop();
}

void SuiteBridgeEmbeddedAgentRuntime::start()
{
    if (worker_)
    {
        worker_->start();
    }
}

void SuiteBridgeEmbeddedAgentRuntime::stop()
{
    if (worker_)
    {
        worker_->stop();
    }
}

bool SuiteBridgeEmbeddedAgentRuntime::running() const
{
    return worker_ && worker_->running();
}

SuiteBridgeEmbeddedAgentHealth
SuiteBridgeEmbeddedAgentRuntime::health() const
{
    SuiteBridgeEmbeddedAgentHealth value;
    value.backendId = config_.backendId;
    value.configured = config_.enabled;
    value.running = running();

    if (worker_)
    {
        value.observation = worker_->snapshot();
    }
    else
    {
        value.observation.state =
            SuiteBridgeObservationState::NotConfigured;
        value.observation.diagnostic = config_.enabled
            ? "Suite Bridge transport unavailable"
            : "Suite Bridge disabled";
        value.observation.started = false;
        value.observation.mutationsEnabled = false;
    }

    return value;
}

std::string SuiteBridgeEmbeddedAgentRuntime::boundedBackendId(
    const std::string& backendId)
{
    if (backendId.empty())
    {
        return "default";
    }

    return backendId.substr(
        0,
        std::min(backendId.size(), MaximumBackendIdLength));
}

void SuiteBridgeEmbeddedAgentRuntime::initializeWorker()
{
    if (!config_.enabled || !transport_)
    {
        return;
    }

    worker_ = std::make_unique<SuiteBridgeObservationWorker>(
        *transport_,
        config_.observation);
}

}
