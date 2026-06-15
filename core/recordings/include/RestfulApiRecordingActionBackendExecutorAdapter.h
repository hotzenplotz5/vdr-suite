#pragma once

#include "HttpResponse.h"
#include "IHttpClient.h"
#include "IRecordingActionBackendExecutorAdapter.h"
#include "RestfulApiRecordingActionBackendConfig.h"
#include "RestfulApiRecordingActionRequestBuilder.h"

#include <string>
#include <utility>

class RestfulApiRecordingActionBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    RestfulApiRecordingActionBackendExecutorAdapter(
        RestfulApiRecordingActionBackendConfig config,
        IHttpClient& httpClient)
        : config_(std::move(config)),
          httpClient_(httpClient)
    {
    }

    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override
    {
        RecordingActionExecutionResult result;
        result.type = payload.type;
        result.backendId = backendId();
        result.recordingId = payload.recordingId;

        if (!isSupportedAction(payload.type)) {
            result.success = false;
            result.message = "restfulapi backend executor action not supported";
            result.errors.push_back("unsupported recording action type");
            return result;
        }

        RestfulApiRecordingActionRequestBuilder requestBuilder;
        const HttpRequest request = buildRequest(requestBuilder, payload);
        const HttpResponse response = httpClient_.execute(request);

        result.success = response.statusCode >= 200 && response.statusCode < 300;
        result.message =
            result.success
                ? "restfulapi backend executor request accepted"
                : "restfulapi backend executor request failed";

        if (!result.success) {
            result.errors.push_back("restfulapi backend returned non-success status");
        }

        return result;
    }

    std::string backendId() const override
    {
        return config_.backendId;
    }

    std::string backendType() const override
    {
        return "restfulapi";
    }

    const RestfulApiRecordingActionBackendConfig& config() const
    {
        return config_;
    }

private:
    static bool isSupportedAction(RecordingActionType type)
    {
        return
            type == RecordingActionType::Move ||
            type == RecordingActionType::Rename ||
            type == RecordingActionType::Delete;
    }

    HttpRequest buildRequest(
        RestfulApiRecordingActionRequestBuilder& requestBuilder,
        const RecordingActionJobPayload& payload) const
    {
        if (payload.type == RecordingActionType::Move) {
            return requestBuilder.buildMoveRequest(config_, payload);
        }

        if (payload.type == RecordingActionType::Rename) {
            return requestBuilder.buildRenameRequest(config_, payload);
        }

        return requestBuilder.buildDeleteRequest(config_, payload);
    }

    RestfulApiRecordingActionBackendConfig config_;
    IHttpClient& httpClient_;
};
