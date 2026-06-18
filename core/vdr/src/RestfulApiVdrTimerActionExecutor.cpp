#include "RestfulApiVdrTimerActionExecutor.h"

#include <string>

RestfulApiVdrTimerActionExecutor::RestfulApiVdrTimerActionExecutor(
    std::string backendId,
    std::string basePath,
    IHttpClient& httpClient)
    : backendId_(backendId),
      basePath_(basePath),
      httpClient_(httpClient)
{
}

VdrTimerActionResult RestfulApiVdrTimerActionExecutor::execute(
    VdrTimerActionType type,
    const VdrTimerOperationRequest& request)
{
    switch (type)
    {
        case VdrTimerActionType::Create:
            return executeBuiltRequest(
                type,
                request,
                requestBuilder_.buildCreateRequest(basePath_, request));

        case VdrTimerActionType::Update:
            return executeBuiltRequest(
                type,
                request,
                requestBuilder_.buildUpdateRequest(basePath_, request));

        case VdrTimerActionType::Delete:
            return executeBuiltRequest(
                type,
                request,
                requestBuilder_.buildDeleteRequest(basePath_, request));

        default:
            return VdrTimerActionResult::failed(
                type,
                request.timerId,
                backendId_,
                "RESTfulAPI timer action type not supported",
                {"unsupported timer action type for RESTfulAPI executor"});
    }
}

VdrTimerActionResult RestfulApiVdrTimerActionExecutor::executeBuiltRequest(
    VdrTimerActionType type,
    const VdrTimerOperationRequest& request,
    const HttpRequest& httpRequest) const
{
    const HttpResponse response =
        httpClient_.execute(httpRequest);

    if (response.statusCode >= 200 && response.statusCode < 300)
    {
        return VdrTimerActionResult::ok(
            type,
            request.timerId,
            backendId_,
            "RESTfulAPI timer action request executed");
    }

    std::string error =
        "RESTfulAPI returned HTTP status " + std::to_string(response.statusCode);

    if (!response.body.empty())
    {
        error += ": " + response.body;
    }

    return VdrTimerActionResult::failed(
        type,
        request.timerId,
        backendId_,
        "RESTfulAPI timer action request failed",
        {error});
}
