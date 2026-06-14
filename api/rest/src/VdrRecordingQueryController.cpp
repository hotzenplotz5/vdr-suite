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
        0,
        0);
}

ApiResponse VdrRecordingQueryController::getRecordings(
    const std::string& title,
    const std::string& path,
    const std::string& sort,
    const std::string& order,
    int limit,
    int offset)
{
    const VdrRecordingSortField sortField =
        parseSortField(sort);

    VdrRecordingQuery query =
        (title.empty() && path.empty() && sortField == VdrRecordingSortField::None)
            ? VdrRecordingQuery::limited(
                  limit,
                  offset)
            : VdrRecordingQuery::sorted(
                  title,
                  path,
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
