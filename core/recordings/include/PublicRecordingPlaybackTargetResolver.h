#pragma once
#include "VdrRecording.h"
#include <string>

class PublicRecordingIdentityRepository;
class VdrRecordingCacheRepository;

// INTERNAL helper only. Callers must authenticate and authorize the
// actor for the exact backend: media.recording.play for resolve(), and
// recordings.view for resolveForRead(). Neither result may be serialized
// directly to a Public-v1 response; always apply the bounded projection.
enum class PublicRecordingPlaybackTargetStatus { invalidRequest, notFound, ready };

struct PublicRecordingPlaybackTargetResult {
    PublicRecordingPlaybackTargetStatus status =
        PublicRecordingPlaybackTargetStatus::notFound;
    VdrRecording recording;
};

class PublicRecordingPlaybackTargetResolver final {
public:
    PublicRecordingPlaybackTargetResolver(
        const PublicRecordingIdentityRepository& identities,
        const VdrRecordingCacheRepository& cache);
    PublicRecordingPlaybackTargetResult resolve(
        const std::string& authorizedBackendId,
        const std::string& publicRecordingId) const;
    // Read-only metadata accepts current recordings that cannot be played:
    // no media-service ID or native title is needed for display projection.
    PublicRecordingPlaybackTargetResult resolveForRead(
        const std::string& authorizedBackendId,
        const std::string& publicRecordingId) const;
private:
    const PublicRecordingIdentityRepository& identities_;
    const VdrRecordingCacheRepository& cache_;
};
