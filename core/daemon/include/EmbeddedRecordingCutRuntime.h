#pragma once

#include "BackendAgentRecordingCutTransport.h"
#include "EmbeddedRecordingCutRepository.h"
#include "IVdrRecordingNativeCutStateResolver.h"
#include "RecordingCutApiRuntime.h"

#include <mutex>
#include <string>

class EmbeddedRecordingCutRuntime
{
public:
    EmbeddedRecordingCutRuntime(
        Database& database,
        std::string backendId,
        vdrsuite::agent::IBackendAgentRecordingCutTransport& transport,
        IVdrRecordingNativeCutStateResolver& resolver);

    bool ensureSchema();

    RecordingCutDispatchResult dispatch(
        const RecordingCutStartRequest& request);

private:
    EmbeddedRecordingCutRepository repository_;
    std::string backendId_;
    vdrsuite::agent::IBackendAgentRecordingCutTransport& transport_;
    IVdrRecordingNativeCutStateResolver& resolver_;
    std::mutex mutex_;
};
