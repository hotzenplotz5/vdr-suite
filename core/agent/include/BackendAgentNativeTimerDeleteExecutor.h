#pragma once

#include "BackendAgentNativeTimerDeleteLocalState.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

enum class BackendAgentNativeTimerDeleteTransportDisposition
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct BackendAgentNativeTimerDeleteTransportRequest
{
    BackendAgentNativeTimerDeleteCommand command;
    std::int64_t localStartingPersistedAt = 0;
};

struct BackendAgentNativeTimerDeleteTransportReply
{
    BackendAgentNativeTimerDeleteTransportDisposition disposition =
        BackendAgentNativeTimerDeleteTransportDisposition::outcomeUnknown;
    std::string evidenceReference;
};

class IBackendAgentNativeTimerDeleteTransport
{
public:
    virtual ~IBackendAgentNativeTimerDeleteTransport() = default;

    virtual bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) = 0;

    virtual BackendAgentNativeTimerDeleteTransportReply deleteTimer(
        const BackendAgentNativeTimerDeleteTransportRequest& request) = 0;
};

struct BackendAgentNativeTimerDeleteExecutorContext
{
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::int64_t now = 0;
};

bool backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerDeleteLocalState& localState,
    const BackendAgentNativeTimerDeleteExecutorContext& context,
    IBackendAgentNativeTimerDeleteTransport& transport,
    BackendAgentNativeTimerDeleteEvidence& evidence,
    std::string& reasonCode);

}
