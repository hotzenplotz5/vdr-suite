#pragma once

#include "BackendAgentNativeTimerCreateLocalState.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

enum class BackendAgentNativeTimerCreateTransportDisposition
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct BackendAgentNativeTimerCreateTransportRequest
{
    BackendAgentNativeTimerCreateCommand command;
    std::int64_t localStartingPersistedAt = 0;
};

struct BackendAgentNativeTimerCreateTransportReply
{
    BackendAgentNativeTimerCreateTransportDisposition disposition =
        BackendAgentNativeTimerCreateTransportDisposition::outcomeUnknown;
    std::string evidenceReference;
};

class IBackendAgentNativeTimerCreateTransport
{
public:
    virtual ~IBackendAgentNativeTimerCreateTransport() = default;

    virtual bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) = 0;

    virtual BackendAgentNativeTimerCreateTransportReply createTimer(
        const BackendAgentNativeTimerCreateTransportRequest& request) = 0;
};

struct BackendAgentNativeTimerCreateExecutorContext
{
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::int64_t now = 0;
};

bool backendAgentNativeTimerCreateExecuteFreshStartingOnce(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerCreateLocalState& localState,
    const BackendAgentNativeTimerCreateExecutorContext& context,
    IBackendAgentNativeTimerCreateTransport& transport,
    BackendAgentNativeTimerCreateEvidence& evidence,
    std::string& reasonCode);

} // namespace vdrsuite::agent
