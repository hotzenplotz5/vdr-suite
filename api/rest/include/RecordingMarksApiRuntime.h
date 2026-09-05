#pragma once

#include "DashboardController.h"
#include "IVdrRecordingNativeMarksResolver.h"
#include "VdrRecording.h"

#include <functional>
#include <mutex>
#include <string>
#include <vector>

enum class RecordingMarksBackendAvailability
{
    Available,
    BackendNotFound,
    CapabilityUnavailable
};

struct RecordingMarksBackendAccess
{
    RecordingMarksBackendAvailability availability =
        RecordingMarksBackendAvailability::BackendNotFound;
    IVdrRecordingNativeMarksResolver* resolver = nullptr;
};

enum class RecordingMarksMutationKind
{
    Add,
    Delete,
    Move,
    Reset,
    Replace
};

struct RecordingMarksMutationRequest
{
    RecordingMarksMutationKind kind = RecordingMarksMutationKind::Add;
    std::string operationId;
    std::string operationRevision;
    std::string recordingKey;
    std::string expectedMarksRevision;
    int sourceFrame = -1;
    int targetFrame = -1;
    std::vector<int> replacementFrames;
    std::string backendId;
};

struct RecordingMarksMutationDispatchResult
{
    bool accepted = false;
    bool replayed = false;
    std::string reasonCode;
    std::string commandId;
    std::string requestFingerprint;
};

struct RecordingMarksBackendWriteAccess
{
    bool allowed = false;
    int statusCode = 503;
    std::string reasonCode = "recording_marks_backend_write_unavailable";
};

class RecordingMarksApiRuntime
{
public:
    using RecordingLookup =
        std::function<std::vector<VdrRecording>(const std::string& backendId)>;
    using BackendResolver =
        std::function<RecordingMarksBackendAccess(const std::string& backendId)>;
    using BackendWritePolicy =
        std::function<RecordingMarksBackendWriteAccess(const std::string& backendId)>;
    using MutationDispatcher =
        std::function<RecordingMarksMutationDispatchResult(
            const RecordingMarksMutationRequest& request)>;

    static RecordingMarksApiRuntime& instance();

    bool configure(
        RecordingLookup recordingLookup,
        BackendResolver backendResolver,
        BackendWritePolicy backendWritePolicy = {},
        MutationDispatcher mutationDispatcher = {});

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
    RecordingMarksApiRuntime() = default;

    mutable std::mutex mutex_;
    RecordingLookup recordingLookup_;
    BackendResolver backendResolver_;
    BackendWritePolicy backendWritePolicy_;
    MutationDispatcher mutationDispatcher_;
};
