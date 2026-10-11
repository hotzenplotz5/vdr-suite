#pragma once

#include "PublicRecordingPlaybackTargetResolver.h"
#include "SecurityIdentity.h"

#include <functional>
#include <string>

// Internal device admission boundary; never serialize the native recording.
// The HTTP caller must first verify the Device credential against persistent
// identity/grant state for EVERY operation (not a browser/CSRF session).
enum class PublicRecordingDevicePlaybackAdmissionStatus {
    unauthenticated, forbidden, invalidRequest, notFound, unavailable, ready
};

struct PublicRecordingDevicePlaybackAdmissionResult {
    PublicRecordingDevicePlaybackAdmissionStatus status =
        PublicRecordingDevicePlaybackAdmissionStatus::unavailable;
    VdrRecording recording;
};

class PublicRecordingDevicePlaybackAdmission final {
public:
    using BackendAvailable = std::function<bool(const std::string&)>;

    PublicRecordingDevicePlaybackAdmission(
        const PublicRecordingPlaybackTargetResolver& resolver,
        BackendAvailable backendAvailable);

    PublicRecordingDevicePlaybackAdmissionResult admit(
        const RequestSecurityContext& verifiedDeviceContext,
        const std::string& backendId,
        const std::string& publicRecordingId) const;

private:
    const PublicRecordingPlaybackTargetResolver& resolver_;
    BackendAvailable backendAvailable_;
};
