#pragma once

#include "VdrRecording.h"

#include <string>
#include <vector>

class PublicRecordingIdentityRepository;

struct PublicRecordingReadItem
{
    std::string recordingId;
    std::string backendId;
    std::string title;
    std::string recordedAt;
    int durationSeconds = 0;
};

struct PublicRecordingReadResult
{
    bool valid = false;
    std::vector<PublicRecordingReadItem> items;
};

// Converts server-side snapshot records into a bounded public representation.
// Caller must have enforced recordings.view for the exact backend scope.
class PublicRecordingCollectionProjection final
{
public:
    explicit PublicRecordingCollectionProjection(
        PublicRecordingIdentityRepository& identities);

    PublicRecordingReadResult project(
        const std::string& authorizedBackendId,
        const std::vector<VdrRecording>& recordings);

private:
    PublicRecordingIdentityRepository& identities_;
};
