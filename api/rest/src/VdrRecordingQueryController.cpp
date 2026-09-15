#include "VdrRecordingQueryController.h"

#include "ManualRecordingMetadataReadModelJson.h"
#include "VdrRecordingQuery.h"
#include "VdrRecordingQueryResult.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryService.h"

#include <map>
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
    ManualMetadataBatchLookup manualMetadataBatchLookup)
    : queryService_(queryService),
      jsonSerializer_(jsonSerializer),
      manualMetadataBatchLookup_(
          std::move(manualMetadataBatchLookup))
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
    const VdrRecordingSortField sortField =
        parseSortField(sort);

    VdrRecordingQuery query =
        (title.empty() && backend.empty() && path.empty() && from.empty() && to.empty() && durationMin <= 0 && durationMax <= 0 && sortField == VdrRecordingSortField::None)
            ? VdrRecordingQuery::limited(
                  limit,
                  offset)
            : VdrRecordingQuery::sortedDurationRanged(
                  title,
                  path,
                  from,
                  to,
                  durationMin,
                  durationMax,
                  limit,
                  offset,
                  sortField,
                  parseSortOrder(order));

    query.setBackendFilter(backend);
    query.setMovieReleaseYears(movieReleaseYearFrom, movieReleaseYearTo);

    VdrRecordingQueryResult result =
        queryService_.queryRecordings(query);

    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";

    if (!manualMetadataBatchLookup_ ||
        result.recordings().empty())
    {
        response.body =
            jsonSerializer_.serialize(result);

        return response;
    }

    const std::string manualBackendId =
        assignmentBackendId(
            backend,
            result);

    const std::map<
        std::string,
        ManualRecordingMetadataAssignment>
        manualAssignments =
            manualMetadataBatchLookup_(
                manualBackendId);

    if (manualAssignments.empty())
    {
        response.body =
            jsonSerializer_.serialize(result);

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
            });

    return response;
}
