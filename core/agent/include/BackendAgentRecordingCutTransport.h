#pragma once

#include "BackendAgentRecordingCut.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

enum class BackendAgentRecordingCutTransportDisposition
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct BackendAgentRecordingCutTransportRequest
{
    BackendAgentRecordingCutCommand command;
    std::int64_t localStartingPersistedAt = 0;
};

struct BackendAgentRecordingCutTransportReply
{
    BackendAgentRecordingCutTransportDisposition disposition =
        BackendAgentRecordingCutTransportDisposition::outcomeUnknown;
    std::string evidenceReference;
};

class IBackendAgentRecordingCutTransport
{
public:
    virtual ~IBackendAgentRecordingCutTransport() = default;

    virtual bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) = 0;

    virtual BackendAgentRecordingCutTransportReply startCut(
        const BackendAgentRecordingCutTransportRequest& request) = 0;
};

}
