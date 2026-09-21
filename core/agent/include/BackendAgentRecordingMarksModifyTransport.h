#pragma once

#include "BackendAgentRecordingMarksModify.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

enum class BackendAgentRecordingMarksModifyTransportDisposition
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct BackendAgentRecordingMarksModifyTransportRequest
{
    BackendAgentRecordingMarksModifyCommand command;
    std::int64_t localStartingPersistedAt = 0;
};

struct BackendAgentRecordingMarksModifyTransportReply
{
    BackendAgentRecordingMarksModifyTransportDisposition disposition =
        BackendAgentRecordingMarksModifyTransportDisposition::outcomeUnknown;
    std::string evidenceReference;
};

class IBackendAgentRecordingMarksModifyTransport
{
public:
    virtual ~IBackendAgentRecordingMarksModifyTransport() = default;

    virtual bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) = 0;

    virtual BackendAgentRecordingMarksModifyTransportReply modifyMarks(
        const BackendAgentRecordingMarksModifyTransportRequest& request) = 0;
};

}
