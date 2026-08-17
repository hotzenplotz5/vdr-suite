#pragma once

#include "BackendAgentNativeTimerModifyLocalState.h"

namespace vdrsuite::agent
{
enum class BackendAgentNativeTimerModifyTransportDisposition {
    rejectedWithoutEffect, acceptedUnverified, outcomeUnknown
};
struct BackendAgentNativeTimerModifyTransportRequest {
    BackendAgentNativeTimerModifyCommand command;
    std::int64_t localStartingPersistedAt=0;
};
struct BackendAgentNativeTimerModifyTransportReply {
    BackendAgentNativeTimerModifyTransportDisposition disposition=
        BackendAgentNativeTimerModifyTransportDisposition::outcomeUnknown;
    std::string evidenceReference;
};
class IBackendAgentNativeTimerModifyTransport {
public:
    virtual ~IBackendAgentNativeTimerModifyTransport()=default;
    virtual bool discoverProvider(BackendAgentLocalProviderFacts&,std::string&)=0;
    virtual BackendAgentNativeTimerModifyTransportReply modifyTimer(
        const BackendAgentNativeTimerModifyTransportRequest&)=0;
};
struct BackendAgentNativeTimerModifyExecutorContext {
    std::string backendId;std::string agentId;std::string agentInstanceId;
    std::uint64_t backendGeneration=0;std::int64_t now=0;
};
bool backendAgentNativeTimerModifyExecuteFreshStartingOnce(
    const BackendAgentCommandAssignment&,
    const BackendAgentNativeTimerModifyLocalState&,
    const BackendAgentNativeTimerModifyExecutorContext&,
    IBackendAgentNativeTimerModifyTransport&,
    BackendAgentNativeTimerModifyEvidence&,std::string&);
}
