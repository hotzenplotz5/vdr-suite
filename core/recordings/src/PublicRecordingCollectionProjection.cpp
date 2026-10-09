#include "PublicRecordingCollectionProjection.h"

#include "PublicRecordingIdentityRepository.h"

#include <algorithm>
#include <utility>

PublicRecordingCollectionProjection::PublicRecordingCollectionProjection(
    PublicRecordingIdentityRepository& identities)
    : identities_(identities)
{
}

PublicRecordingReadResult PublicRecordingCollectionProjection::project(
    const std::string& authorizedBackendId,
    const std::vector<VdrRecording>& recordings)
{
    PublicRecordingReadResult result;
    if (authorizedBackendId.empty() || authorizedBackendId == "*")
        return result;

    for (const VdrRecording& source : recordings)
    {
        if (source.backendId != authorizedBackendId ||
            source.backendNativeId.empty() ||
            source.title.empty() ||
            source.durationSeconds < 0)
            return {};

        const auto publicId = identities_.resolveOrCreate(
            authorizedBackendId, source.backendNativeId);
        if (!publicId.has_value())
            return {};

        PublicRecordingReadItem item;
        item.recordingId = *publicId;
        item.backendId = source.backendId;
        item.title = source.title;
        item.recordedAt = source.startTime;
        item.durationSeconds = source.durationSeconds;
        item.durationKnown = source.recordingDurationKnown;
        result.items.push_back(std::move(item));
    }

    std::sort(result.items.begin(), result.items.end(),
        [](const PublicRecordingReadItem& a,
           const PublicRecordingReadItem& b)
        {
            if (a.recordedAt != b.recordedAt)
                return a.recordedAt > b.recordedAt;
            return a.recordingId < b.recordingId;
        });

    result.valid = true;
    return result;
}
