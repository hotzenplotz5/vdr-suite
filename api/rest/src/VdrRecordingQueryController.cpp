#include "VdrRecordingQueryController.h"

#include "VdrRecordingQuery.h"
#include "VdrRecordingQueryResult.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryService.h"

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
        0,
        0);
}

ApiResponse VdrRecordingQueryController::getRecordings(
    const std::string& title,
    const std::string& path,
    int limit,
    int offset)
{
    VdrRecordingQuery query =
        (title.empty() && path.empty())
            ? VdrRecordingQuery::limited(
                  limit,
                  offset)
            : VdrRecordingQuery::filtered(
                  title,
                  path,
                  limit,
                  offset);

    VdrRecordingQueryResult result =
        queryService_.queryRecordings(query);

    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = jsonSerializer_.serialize(result);

    return response;
}
