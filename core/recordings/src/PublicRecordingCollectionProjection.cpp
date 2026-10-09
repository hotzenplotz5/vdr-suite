#include "PublicRecordingCollectionProjection.h"

#include "PublicRecordingIdentityRepository.h"

#include <algorithm>
#include <utility>

namespace
{
constexpr std::size_t MaximumDescriptionBytes = 2048U;

std::string boundedDescription(const VdrRecording& source)
{
    const std::string& original = !source.metadata.provider.overview.empty()
        ? source.metadata.provider.overview
        : !source.metadata.native.description.empty()
            ? source.metadata.native.description
            : source.metadata.native.shortText;
    if (original.size() <= MaximumDescriptionBytes) return original;
    std::size_t limit = MaximumDescriptionBytes;
    // Never truncate a multibyte UTF-8 sequence in the middle.
    while (limit > 0U &&
        (static_cast<unsigned char>(original[limit]) & 0xc0U) == 0x80U)
        --limit;
    return original.substr(0U, limit);
}
} // namespace

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
        item.description = boundedDescription(source);
        item.sizeMb = std::max(0LL, source.sizeMb);
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
