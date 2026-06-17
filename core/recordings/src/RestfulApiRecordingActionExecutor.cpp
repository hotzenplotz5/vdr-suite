#include "RestfulApiRecordingActionExecutor.h"

RestfulApiRecordingActionExecutor::RestfulApiRecordingActionExecutor(
    std::string backendId,
    std::string backendType,
    RestfulApiRecordingActionBackendConfig config,
    IHttpClient& httpClient)
    : backendId_(backendId),
      backendType_(backendType),
      config_(config),
      httpClient_(httpClient)
{
}

std::string RestfulApiRecordingActionExecutor::backendId() const
{
    return backendId_;
}

std::string RestfulApiRecordingActionExecutor::backendType() const
{
    return backendType_;
}

RecordingActionCapabilitySet RestfulApiRecordingActionExecutor::capabilities() const
{
    RecordingActionCapabilityContract contract;
    return contract.restfulApiDefaultCapabilities();
}

RecordingActionExecutionResult RestfulApiRecordingActionExecutor::execute(
    const RecordingActionJobPayload& payload)
{
    switch (payload.type)
    {
        case RecordingActionType::Move:
            return executeBuiltRequest(
                payload,
                requestBuilder_.buildMoveRequest(config_, payload));

        case RecordingActionType::Rename:
            return executeBuiltRequest(
                payload,
                requestBuilder_.buildRenameRequest(config_, payload));

        case RecordingActionType::Delete:
            return executeBuiltRequest(
                payload,
                requestBuilder_.buildDeleteRequest(config_, payload));

        default:
            return RecordingActionExecutionResult::failed(
                payload.type,
                payload.recordingId,
                payload.backendId,
                "RESTfulAPI recording action type not supported",
                {"unsupported recording action type for RESTfulAPI executor"});
    }
}

RecordingActionExecutionResult RestfulApiRecordingActionExecutor::executeBuiltRequest(
    const RecordingActionJobPayload& payload,
    const HttpRequest& request) const
{
    const HttpResponse response =
        httpClient_.execute(request);

    if (response.statusCode >= 200 &&
        response.statusCode < 300)
    {
        return RecordingActionExecutionResult::ok(
            payload.type,
            payload.recordingId,
            payload.backendId,
            "RESTfulAPI recording action request executed");
    }

    return RecordingActionExecutionResult::failed(
        payload.type,
        payload.recordingId,
        payload.backendId,
        "RESTfulAPI recording action request failed",
        {response.body.empty()
            ? "RESTfulAPI returned non-success status"
            : response.body});
}
