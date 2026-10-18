#pragma once
#include "VdrRecording.h"
#include <string>

class PublicRecordingIdentityRepository;
class VdrRecordingCacheRepository;

// INTERNAL helper only. Caller MUST have authenticated and authorized the
// actor for media.recording.play on this exact backend before calling.
// Result is VDR-native; never serialize it into a Public-v1 response.
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
private:
    const PublicRecordingIdentityRepository& identities_;
    const VdrRecordingCacheRepository& cache_;
};
