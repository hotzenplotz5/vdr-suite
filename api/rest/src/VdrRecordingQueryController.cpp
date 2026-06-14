#include "VdrRecordingQueryController.h"

#include "VdrRecordingQuery.h"
#include "VdrRecordingQueryResult.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryService.h"

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
}

VdrRecordingQueryController::VdrRecordingQueryController(
    VdrRecordingQueryService& queryService,
    VdrRecordingQueryResultJsonSerializer& jsonSerializer)
    : queryService_(queryService),
      jsonSerializer_(jsonSerializer)
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
    const VdrRecordingSortField sortField =
        parseSortField(sort);

    VdrRecordingQuery query =
        (title.empty() && path.empty() && from.empty() && to.empty() && durationMin <= 0 && durationMax <= 0 && sortField == VdrRecordingSortField::None)
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

    VdrRecordingQueryResult result =
        queryService_.queryRecordings(query);

    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = jsonSerializer_.serialize(result);

    return response;
}
