#include "VdrRecordingQueryController.h"

#include "ManualRecordingMetadataReadModelJson.h"
#include "VdrRecordingQuery.h"
#include "VdrRecordingQueryResult.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryService.h"
#include "VdrRecordingQueryMatcher.h"
#include "VdrRecordingMetadataJsonSerializer.h"

#include <algorithm>
#include <map>
#include <tuple>
#include <utility>

namespace
{
VdrRecordingSortField parseSortField(
    const std::string& value)
{
    if (value == "title")
    {
        return VdrRecordingSortField::Title;
    }

    if (value == "startTime")
    {
        return VdrRecordingSortField::StartTime;
    }

    if (value == "duration")
    {
        return VdrRecordingSortField::Duration;
    }

    if (value == "size")
    {
        return VdrRecordingSortField::Size;
    }

    return VdrRecordingSortField::None;
}

VdrRecordingSortOrder parseSortOrder(
    const std::string& value)
{
    if (value == "desc")
    {
        return VdrRecordingSortOrder::Descending;
    }

    return VdrRecordingSortOrder::Ascending;
}

const ManualRecordingMetadataAssignment* findManualAssignment(
    const std::map<
        std::string,
        ManualRecordingMetadataAssignment>& assignments,
    const VdrRecording& recording)
{
    for (const std::string* key : {
             &recording.backendNativeId,
             &recording.path,
             &recording.id})
    {
        if (key->empty())
        {
            continue;
        }

        const auto match =
            assignments.find(*key);

        if (match != assignments.end())
        {
            return &match->second;
        }
    }

    return nullptr;
}

std::string assignmentBackendId(
    const std::string& requestedBackendId,
    const VdrRecordingQueryResult& result)
{
    if (!requestedBackendId.empty())
    {
        return requestedBackendId;
    }

    for (const VdrRecording& recording :
         result.recordings())
    {
        if (!recording.backendId.empty())
        {
            return recording.backendId;
        }
    }

    return "default";
}

// Ordering must use the same effective release date as the serialized
// recording. Manual locked assignments are applied by this controller,
// so ordering the raw catalog in the generic query service is insufficient.
std::string trimmed(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

VdrRecordingQueryResult movieReleasePage(
    const VdrRecordingQueryResult& all,
    const VdrRecordingQuery& query,
    const std::map<std::string, ManualRecordingMetadataAssignment>& assignments,
    int limit, int offset, bool descending)
{
    struct Entry
    {
        const VdrRecording* recording;
        std::string date;
        std::string start;
        std::string title;
        std::string id;
    };
    std::vector<Entry> entries;
    entries.reserve(all.recordings().size());
    for (const auto& recording : all.recordings())
    {
        const auto* assignment = findManualAssignment(assignments, recording);
        const bool manual = assignment && assignment->found && assignment->relationshipLocked;
        if (manual)
        {
            // Preserve the existing native prefilter, then the same effective
            // movie/year predicate that Home previously applied to the overlay.
            auto effective = recording;
            effective.metadata.provider.contentKind = assignment->mediaType == "movie"
                ? VdrRecordingContentKind::Movie : VdrRecordingContentKind::Unknown;
            effective.metadata.provider.releaseDate = assignment->releaseDate;
            if (!VdrRecordingQueryMatcher{}.matches(effective, query)) continue;
        }
        auto title = manual
            ? (assignment->title.empty() ? recording.title : assignment->title)
            : VdrRecordingMetadataJsonSerializer::presentationTitle(recording);
        entries.push_back({&recording,
            trimmed(manual ? assignment->releaseDate : recording.metadata.provider.releaseDate),
            trimmed(recording.startTime), trimmed(title), recording.id});
    }
    std::sort(entries.begin(), entries.end(), [descending](const Entry& a, const Entry& b)
    {
        if (a.date != b.date) return descending ? a.date > b.date : a.date < b.date;
        if (a.start != b.start) return descending ? a.start > b.start : a.start < b.start;
        // Locale-independent stable ties across pagination requests.
        return std::tie(a.title, a.id) < std::tie(b.title, b.id);
    });
    const int total = static_cast<int>(entries.size());
    const int begin = std::min(total, std::max(0, offset));
    const int count = limit > 0 ? std::min(limit, total - begin) : total - begin;
    std::vector<VdrRecording> page;
    page.reserve(static_cast<std::size_t>(count));
    for (int i = begin; i < begin + count; ++i)
        page.push_back(*entries[static_cast<std::size_t>(i)].recording);
    return VdrRecordingQueryResult(std::move(page), total, limit, offset);
}
}

VdrRecordingQueryController::VdrRecordingQueryController(
    VdrRecordingQueryService& queryService,
    VdrRecordingQueryResultJsonSerializer& jsonSerializer)
    : VdrRecordingQueryController(
          queryService,
          jsonSerializer,
          ManualMetadataBatchLookup{})
{
}

VdrRecordingQueryController::VdrRecordingQueryController(
    VdrRecordingQueryService& queryService,
    VdrRecordingQueryResultJsonSerializer& jsonSerializer,
    ManualMetadataBatchLookup manualMetadataBatchLookup,
    PublicRecordingIdLookup publicRecordingIdLookup)
    : queryService_(queryService),
      jsonSerializer_(jsonSerializer),
      manualMetadataBatchLookup_(
          std::move(manualMetadataBatchLookup)),
      publicRecordingIdLookup_(std::move(publicRecordingIdLookup))
{
}

ApiResponse VdrRecordingQueryController::getRecordings()
{
    return getRecordings(
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        0,
        0,
        0,
        0);
}

ApiResponse VdrRecordingQueryController::getRecordings(
    const std::string& title,
    const std::string& path,
    const std::string& sort,
    const std::string& order,
    const std::string& from,
    const std::string& to,
    int durationMin,
    int durationMax,
    int limit,
    int offset)
{
    return getRecordings(
        title,
        "",
        path,
        sort,
        order,
        from,
        to,
        durationMin,
        durationMax,
        limit,
        offset);
}

ApiResponse VdrRecordingQueryController::getRecordings(
    const std::string& title,
    const std::string& backend,
    const std::string& path,
    const std::string& sort,
    const std::string& order,
    const std::string& from,
    const std::string& to,
    int durationMin,
    int durationMax,
    int limit,
    int offset,
    int movieReleaseYearFrom,
    int movieReleaseYearTo)
{
    const bool releaseOrder = sort == "movieReleaseDate";
    const int queryLimit = releaseOrder ? 0 : limit;
    const int queryOffset = releaseOrder ? 0 : offset;
    const VdrRecordingSortField sortField =
        parseSortField(sort);

    VdrRecordingQuery query =
        (title.empty() && backend.empty() && path.empty() && from.empty() && to.empty() && durationMin <= 0 && durationMax <= 0 && sortField == VdrRecordingSortField::None)
            ? VdrRecordingQuery::limited(
                  queryLimit,
                  queryOffset)
            : VdrRecordingQuery::sortedDurationRanged(
                  title,
                  path,
                  from,
                  to,
                  durationMin,
                  durationMax,
                  queryLimit,
                  queryOffset,
                  sortField,
                  parseSortOrder(order));

    query.setBackendFilter(backend);
    query.setMovieReleaseYears(movieReleaseYearFrom, movieReleaseYearTo);

    VdrRecordingQueryResult result =
        queryService_.queryRecordings(query);

    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";

    const auto manualAssignments =
        manualMetadataBatchLookup_ && !result.recordings().empty()
            ? manualMetadataBatchLookup_(assignmentBackendId(backend, result))
            : std::map<std::string, ManualRecordingMetadataAssignment>{};

    if (releaseOrder)
        result = movieReleasePage(result, query, manualAssignments, limit, offset,
                                  order == "desc");

    const auto publicIdLookup = [this](const VdrRecording& recording)
    {
        if (!publicRecordingIdLookup_ || recording.backendNativeId.empty())
            return std::string{};
        return publicRecordingIdLookup_(recording.backendId,
                                        recording.backendNativeId);
    };

    if (manualAssignments.empty())
    {
        response.body =
            jsonSerializer_.serialize(result, {}, publicIdLookup);

        return response;
    }

    response.body =
        jsonSerializer_.serialize(
            result,
            [&manualAssignments](
                const VdrRecording& recording)
            {
                const auto* assignment =
                    findManualAssignment(
                        manualAssignments,
                        recording);

                if (assignment == nullptr ||
                    !assignment->found ||
                    !assignment->relationshipLocked)
                {
                    return std::string{};
                }

                return
                    vdrsuite::rest::
                    manual_recording_metadata_json::
                    serialize(
                        *assignment,
                        recording.backendNativeId);
            }, publicIdLookup);

    return response;
}
