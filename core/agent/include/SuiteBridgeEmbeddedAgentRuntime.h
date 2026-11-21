#pragma once

#include "ISuiteBridgeLocalTransport.h"
#include "SuiteBridgeObservation.h"
#include "SuiteBridgeObservationWorker.h"
#include "SuiteBridgeSvdrpTransport.h"

#include <memory>
#include <string>

namespace vdrsuite::agent
{

struct SuiteBridgeEmbeddedAgentConfig
{
    std::string backendId = "default";
    bool enabled = false;
    SuiteBridgeSvdrpTransportConfig transport;
    SuiteBridgeObservationConfig observation;
};

struct SuiteBridgeEmbeddedAgentHealth
{
    std::string backendId;
    bool configured = false;
    bool running = false;
    SuiteBridgeObservationSnapshot observation;
};

class SuiteBridgeEmbeddedAgentRuntime
{
public:
    explicit SuiteBridgeEmbeddedAgentRuntime(
        SuiteBridgeEmbeddedAgentConfig config = {});

    SuiteBridgeEmbeddedAgentRuntime(
        SuiteBridgeEmbeddedAgentConfig config,
        std::unique_ptr<ISuiteBridgeLocalTransport> transport);

    ~SuiteBridgeEmbeddedAgentRuntime();

    void start();
    void stop();

    bool running() const;
    SuiteBridgeEmbeddedAgentHealth health() const;

private:
    static std::string boundedBackendId(
        const std::string& backendId);

    void initializeWorker();

    SuiteBridgeEmbeddedAgentConfig config_;
    std::unique_ptr<ISuiteBridgeLocalTransport> transport_;
    std::unique_ptr<SuiteBridgeObservationWorker> worker_;
};

}
