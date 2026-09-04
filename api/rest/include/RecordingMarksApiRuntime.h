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

class RecordingMarksApiRuntime
{
public:
    using RecordingLookup =
        std::function<std::vector<VdrRecording>(const std::string& backendId)>;
    using BackendResolver =
        std::function<RecordingMarksBackendAccess(const std::string& backendId)>;

    static RecordingMarksApiRuntime& instance();

    bool configure(
        RecordingLookup recordingLookup,
        BackendResolver backendResolver);

    void reset();
    bool configured() const;

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

private:
    RecordingMarksApiRuntime() = default;

    mutable std::mutex mutex_;
    RecordingLookup recordingLookup_;
    BackendResolver backendResolver_;
};
