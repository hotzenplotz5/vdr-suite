#pragma once

#include "GlobalSearchResult.h"
#include "PublicRecordingDisplayName.h"
#include "PublicApiRuntime.h"
#include "VdrRecording.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Adapter only: the Web GlobalSearchService owns all query and ranking logic.
// No re-search, substring matcher, native path or provider query belongs here.
namespace PublicSearchProjection
{
using RecordingIdLookup = std::function<std::optional<std::string>(
    const std::string&, const std::string&)>;

inline bool validRecordingId(const std::string& id)
{
    return id.size() == 36U && id.compare(0, 4, "rec_") == 0 &&
        std::all_of(id.begin() + 4, id.end(),
            [](unsigned char ch) {
                return (ch >= '0' && ch <= '9') ||
                    (ch >= 'a' && ch <= 'f');
            });
}

inline PublicSearchResult project(
    const PublicSearchRequest& request,
    const GlobalSearchResult& searched,
    const std::vector<VdrRecording>& currentRecordings,
    const RecordingIdLookup& recordingIdLookup)
{
    PublicSearchResult result;
    const std::size_t pageCount =
        request.epg ? searched.epg.size() : searched.recordings.size();
    if (searched.status != "ready" ||
        searched.backendId != request.backendId ||
        request.limit == 0U || request.limit > 30U ||
        pageCount > request.limit)
        return result;

    // Keep the existing, bounded Web search order and effective titles.
    // Only the authorized backend's public fields may leave the adapter.
    if (request.epg)
    {
        for (const auto& event : searched.epg)
        {
            if (event.backendId != request.backendId ||
                event.channelId.empty() || event.eventId.empty() ||
                event.title.empty() || event.startTime.empty() ||
                event.endTime.empty())
                continue;

            PublicSearchItem item;
            item.backendId = request.backendId;
            item.channelId = event.channelId;
            item.eventId = event.eventId;
            item.title = event.title;
            item.subtitle = event.subtitle;
            item.startTime = event.startTime;
            item.endTime = event.endTime;
            result.items.push_back(std::move(item));
        }
    }
    else
    {
        for (const auto& recording : searched.recordings)
        {
            if (recording.backendId != request.backendId ||
                recording.backendNativeId.empty() ||
                recording.title.empty())
                continue;
            // Never disclose a deleted recording from an obsolete SQL index.
            const auto live = std::find_if(
                currentRecordings.begin(), currentRecordings.end(),
                [&recording, &request](const VdrRecording& candidate) {
                    return candidate.backendId == request.backendId &&
                        candidate.backendNativeId == recording.backendNativeId;
                });
            if (live == currentRecordings.end())
                continue;
            const auto publicId = recordingIdLookup(
                request.backendId, recording.backendNativeId);
            if (!publicId.has_value() || !validRecordingId(*publicId))
                continue;

            PublicSearchItem item;
            item.backendId = request.backendId;
            item.recordingId = *publicId;
            // Search matches manual metadata titles too. The native VDR title
            // is not the canonical search title and must not replace it.
            const auto display = PublicRecordingDisplayName::project(*live);
            item.title = recording.title == live->title ? display.title : recording.title;
            item.edited = display.edited;
            item.startTime = recording.startTime;
            result.items.push_back(std::move(item));
        }
    }

    // A single stale or unprojectable entry must not turn valid results
    // into HTTP 503. The dropped entries are not exposed or counted on this
    // page; the existing canonical search supplies all ordering and totals.
    const int total = request.epg ? searched.epgTotal : searched.recordingTotal;
    const std::size_t canonicalTotal = static_cast<std::size_t>(std::max(0, total));
    const std::size_t dropped = pageCount - result.items.size();
    const std::size_t reconciledTotal =
        canonicalTotal >= dropped ? canonicalTotal - dropped : 0U;
    const std::size_t shown = request.offset + result.items.size();
    result.totalCount = std::max(shown, reconciledTotal);
    result.hasMore = shown < result.totalCount;
    result.status = PublicSearchStatus::ok;
    return result;
}
} // namespace PublicSearchProjection
