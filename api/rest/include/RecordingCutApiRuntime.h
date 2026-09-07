#pragma once

#include "DashboardController.h"
#include "IVdrRecordingNativeCutStateResolver.h"
#include "VdrRecording.h"

#include <functional>
#include <mutex>
#include <string>
#include <vector>

enum class RecordingCutBackendAvailability
{
    Available,
    BackendNotFound,
    CapabilityUnavailable
};

struct RecordingCutBackendAccess
{
    RecordingCutBackendAvailability availability =
        RecordingCutBackendAvailability::BackendNotFound;
    IVdrRecordingNativeCutStateResolver* resolver = nullptr;
};

struct RecordingCutStartRequest
{
    std::string operationId;
    std::string operationRevision;
    std::string recordingKey;
    std::string expectedMarksRevision;
    std::string backendId;
    bool replayOnly = false;
};

struct RecordingCutDispatchResult
{
    bool accepted = false;
    bool replayed = false;
    bool verified = false;
    std::string reasonCode;
    std::string commandId;
    std::string requestFingerprint;
    std::string editedRecordingKey;
};

struct RecordingCutBackendWriteAccess
{
    bool allowed = false;
    int statusCode = 503;
    std::string reasonCode = "recording_cut_backend_write_unavailable";
};

class RecordingCutApiRuntime
{
public:
    using RecordingLookup =
        std::function<std::vector<VdrRecording>(const std::string& backendId)>;
    using BackendResolver =
        std::function<RecordingCutBackendAccess(const std::string& backendId)>;
    using BackendWritePolicy =
        std::function<RecordingCutBackendWriteAccess(const std::string& backendId)>;
    using StartDispatcher =
        std::function<RecordingCutDispatchResult(const RecordingCutStartRequest& request)>;

    static RecordingCutApiRuntime& instance();

    bool configure(
        RecordingLookup recordingLookup,
        BackendResolver backendResolver,
        BackendWritePolicy backendWritePolicy = {},
        StartDispatcher startDispatcher = {});

    void reset();
    bool configured() const;
    bool mutationConfigured() const;

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        ApiResponse& response) const;

private:
    RecordingCutApiRuntime() = default;

    mutable std::mutex mutex_;
    RecordingLookup recordingLookup_;
    BackendResolver backendResolver_;
    BackendWritePolicy backendWritePolicy_;
    StartDispatcher startDispatcher_;
};
