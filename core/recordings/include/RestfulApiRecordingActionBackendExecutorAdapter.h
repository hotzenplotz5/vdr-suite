#pragma once

#include "HttpResponse.h"
#include "IHttpClient.h"
#include "IRecordingActionBackendExecutorAdapter.h"
#include "RestfulApiRecordingActionBackendConfig.h"
#include "RestfulApiRecordingActionRequestBuilder.h"

#include <map>
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
            result.errors.push_back(
                "unsupported recording action type for restfulapi backend executor");
            return result;
        }

        if (!validatePayload(payload, result)) {
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
            result.errors.push_back(
                "restfulapi backend returned HTTP status " +
                std::to_string(response.statusCode));

            if (!response.body.empty()) {
                result.errors.push_back(response.body);
            }
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

    static bool hasParameter(
        const std::map<std::string, std::string>& parameters,
        const std::string& name)
    {
        const auto it = parameters.find(name);

        return it != parameters.end() && !it->second.empty();
    }

    static bool validatePayload(
        const RecordingActionJobPayload& payload,
        RecordingActionExecutionResult& result)
    {
        if (payload.recordingId.empty()) {
            result.success = false;
            result.message = "restfulapi backend executor payload invalid";
            result.errors.push_back("recordingId is required");
            return false;
        }

        if (payload.type == RecordingActionType::Move &&
            !hasParameter(payload.parameters, "targetPath")) {
            result.success = false;
            result.message = "restfulapi backend executor payload invalid";
            result.errors.push_back("targetPath is required for move");
            return false;
        }

        if (payload.type == RecordingActionType::Rename &&
            !hasParameter(payload.parameters, "newName")) {
            result.success = false;
            result.message = "restfulapi backend executor payload invalid";
            result.errors.push_back("newName is required for rename");
            return false;
        }

        return true;
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
