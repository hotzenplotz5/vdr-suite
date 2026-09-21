#pragma once

#include "EmbeddedRecordingMarksRepository.h"
#include "RecordingMarksApiRuntime.h"
#include "BackendAgentRecordingMarksModifyTransport.h"

#include <mutex>
#include <string>

// Durable control-plane journal for the configured embedded SuiteBridge path.
// Only the native provider changes marks; this journal stores command evidence.
class EmbeddedRecordingMarksRuntime
{
public:
    EmbeddedRecordingMarksRuntime(Database& database, std::string backendId,
        vdrsuite::agent::IBackendAgentRecordingMarksModifyTransport& transport,
        IVdrRecordingNativeMarksResolver& resolver);
    bool ensureSchema();
    RecordingMarksMutationDispatchResult dispatch(const RecordingMarksMutationRequest& request);

private:
    EmbeddedRecordingMarksRepository repository_;
    std::string backendId_;
    vdrsuite::agent::IBackendAgentRecordingMarksModifyTransport& transport_;
    IVdrRecordingNativeMarksResolver& resolver_;
    std::mutex mutex_;
};
