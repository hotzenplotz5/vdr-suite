#pragma once

#include "HttpResponse.h"
#include "IHttpClient.h"
#include "IRecordingActionBackendExecutorAdapter.h"
#include "RestfulApiRecordingActionBackendConfig.h"
#include "RecordingActionCapabilityContract.h"
#include "RestfulApiRecordingActionRequestBuilder.h"
#include "RestfulApiRecordingTrashRequestBuilder.h"
#include "RestfulApiRecordingTrashResponseParser.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

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
        RecordingActionExecutionResult result = makeResult(payload);

        if (!isSupportedAction(payload.type)) {
            result.message = "restfulapi backend executor action not supported";
            result.errors.push_back(
                "unsupported recording action type for restfulapi backend executor");
            return result;
        }

        if (!validatePayload(payload, result)) {
            return result;
        }

        if (!enforceReadOnlyPolicy(result)) {
            return result;
        }

        if (!enforceExecutionPolicy(payload, result)) {
            return result;
        }

        if (payload.type == RecordingActionType::Delete) {
            return executeTrashWorkflow(payload);
        }

        RestfulApiRecordingActionRequestBuilder requestBuilder;
        return executeSingleRequest(
            payload,
            buildRequest(requestBuilder, payload));
    }

    std::string backendId() const override
    {
        return config_.backendId;
    }

    std::string backendType() const override
    {
        return "restfulapi";
    }

    RecordingActionCapabilitySet capabilities() const
    {
        RecordingActionCapabilityContract contract;
        return contract.restfulApiDefaultCapabilities();
    }

    bool supportsAction(
        RecordingActionType type) const
    {
        RecordingActionCapabilityContract contract;
        const RecordingActionCapabilityCheckResult result =
            contract.check(type, capabilities());
        return result.supported;
    }

    const RestfulApiRecordingActionBackendConfig& config() const
    {
        return config_;
    }

private:
    static RecordingActionExecutionResult makeResult(
        const RecordingActionJobPayload& payload)
    {
        RecordingActionExecutionResult result;
        result.type = payload.type;
        result.backendId = payload.backendId.empty()
            ? "default"
            : payload.backendId;
        result.recordingId = payload.recordingId;

        const auto backendNativeId =
            payload.parameters.find("backendNativeId");
        if (backendNativeId != payload.parameters.end()) {
            result.backendNativeId = backendNativeId->second;
        }

        const auto recordingPath =
            payload.parameters.find("recordingPath");
        if (recordingPath != payload.parameters.end()) {
            result.recordingPath = recordingPath->second;
        }

        return result;
    }

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
            result.message = "restfulapi backend executor payload invalid";
            result.errors.push_back("recordingId is required");
            return false;
        }

        if (payload.type == RecordingActionType::Move &&
            !hasParameter(payload.parameters, "targetPath")) {
            result.message = "restfulapi backend executor payload invalid";
            result.errors.push_back("targetPath is required for move");
            return false;
        }

        if (payload.type == RecordingActionType::Rename &&
            !hasParameter(payload.parameters, "newName")) {
            result.message = "restfulapi backend executor payload invalid";
            result.errors.push_back("newName is required for rename");
            return false;
        }

        return true;
    }

    bool enforceReadOnlyPolicy(
        RecordingActionExecutionResult& result) const
    {
        if (!config_.readOnly) {
            return true;
        }

        result.message = "restfulapi backend executor backend is read-only";
        result.errors.push_back(
            "recording action execution is blocked by read-only backend config");
        return false;
    }

    bool enforceExecutionPolicy(
        const RecordingActionJobPayload& payload,
        RecordingActionExecutionResult& result) const
    {
        if (payload.dryRun || config_.allowExecution) {
            return true;
        }

        result.message = "restfulapi backend executor execution disabled";
        result.errors.push_back(
            "real recording action execution is disabled by restfulapi backend config");
        return false;
    }

    RecordingActionExecutionResult executeSingleRequest(
        const RecordingActionJobPayload& payload,
        const HttpRequest& request) const
    {
        RecordingActionExecutionResult result = makeResult(payload);
        const HttpResponse response = httpClient_.execute(request);

        result.upstreamHttpStatus = response.statusCode;
        result.upstreamEndpoint = request.url;
        result.upstreamResponseBody = response.body;
        result.success = isSuccessful(response);
        result.message = result.success
            ? "restfulapi backend executor request accepted"
            : "restfulapi backend executor request failed";

        if (!result.success) {
            appendHttpFailure(result, response);
        }

        return result;
    }

    RecordingActionExecutionResult executeTrashWorkflow(
        const RecordingActionJobPayload& payload) const
    {
        RecordingActionExecutionResult result = makeResult(payload);
        RestfulApiRecordingTrashRequestBuilder builder;

        const HttpRequest previewRequest =
            builder.buildPreviewRequest(config_, payload);
        const HttpResponse previewResponse =
            httpClient_.execute(previewRequest);

        setUpstream(result, previewRequest, previewResponse);
        if (!isSuccessful(previewResponse)) {
            result.message = "restfulapi recording trash preview request failed";
            appendHttpFailure(result, previewResponse);
            return result;
        }

        const RestfulApiRecordingTrashPreviewResponse preview =
            RestfulApiRecordingTrashResponseParser::parsePreview(
                previewResponse.body);

        if (!preview.parsed) {
            result.message = "restfulapi recording trash preview response invalid";
            result.errors.push_back(
                "recording trash preview response is missing required fields");
            return result;
        }

        result.warnings = preview.warnings;
        if (!preview.executable) {
            result.message = "restfulapi recording trash preview blocked";
            appendBlockers(result, preview.blockers);
            return result;
        }

        if (payload.dryRun) {
            result.success = true;
            result.message = "restfulapi recording trash preview ready";
            return result;
        }

        const HttpRequest validateRequest =
            builder.buildValidateRequest(
                config_,
                payload,
                preview.recordingsState,
                preview.timersState);
        const HttpResponse validateResponse =
            httpClient_.execute(validateRequest);

        setUpstream(result, validateRequest, validateResponse);
        if (!isSuccessful(validateResponse)) {
            result.message = "restfulapi recording trash validation request failed";
            appendHttpFailure(result, validateResponse);
            return result;
        }

        const RestfulApiRecordingTrashValidateResponse validation =
            RestfulApiRecordingTrashResponseParser::parseValidate(
                validateResponse.body);

        if (!validation.parsed) {
            result.message = "restfulapi recording trash validation response invalid";
            result.errors.push_back(
                "recording trash validation response is missing status");
            return result;
        }

        appendWarnings(result, validation.warnings);
        if (validation.status != "ready") {
            result.message = "restfulapi recording trash validation blocked";
            appendBlockers(result, validation.blockers);
            if (result.errors.empty()) {
                result.errors.push_back(
                    "recording trash validation status is " + validation.status);
            }
            return result;
        }

        const HttpRequest executeRequest =
            builder.buildExecuteRequest(
                config_,
                payload,
                preview.recordingsState,
                preview.timersState);
        const HttpResponse executeResponse =
            httpClient_.execute(executeRequest);

        setUpstream(result, executeRequest, executeResponse);
        if (!isSuccessful(executeResponse)) {
            result.message = "restfulapi recording trash execution request failed";
            appendHttpFailure(result, executeResponse);
            return result;
        }

        const RestfulApiRecordingTrashExecuteResponse execution =
            RestfulApiRecordingTrashResponseParser::parseExecute(
                executeResponse.body);

        if (!execution.parsed) {
            result.message = "restfulapi recording trash execution response invalid";
            result.errors.push_back(
                "recording trash execution response is missing status");
            return result;
        }

        result.success =
            execution.status == "trashed" ||
            execution.status == "already-trashed";
        result.message = execution.message.empty()
            ? "restfulapi recording trash " + execution.status
            : execution.message;

        if (!result.success) {
            result.errors.push_back(
                "recording trash execution status is " + execution.status);
        }

        return result;
    }

    static bool isSuccessful(const HttpResponse& response)
    {
        return response.statusCode >= 200 && response.statusCode < 300;
    }

    static void setUpstream(
        RecordingActionExecutionResult& result,
        const HttpRequest& request,
        const HttpResponse& response)
    {
        result.upstreamHttpStatus = response.statusCode;
        result.upstreamEndpoint = request.url;
        result.upstreamResponseBody = response.body;
    }

    static void appendHttpFailure(
        RecordingActionExecutionResult& result,
        const HttpResponse& response)
    {
        result.errors.push_back(
            "restfulapi backend returned HTTP status " +
            std::to_string(response.statusCode));

        if (!response.body.empty()) {
            result.errors.push_back(response.body);
        }
    }

    static void appendWarnings(
        RecordingActionExecutionResult& result,
        const std::vector<std::string>& warnings)
    {
        result.warnings.insert(
            result.warnings.end(),
            warnings.begin(),
            warnings.end());
    }

    static void appendBlockers(
        RecordingActionExecutionResult& result,
        const std::vector<std::string>& blockers)
    {
        for (const std::string& blocker : blockers) {
            result.errors.push_back("recording trash blocker: " + blocker);
        }
    }

    HttpRequest buildRequest(
        RestfulApiRecordingActionRequestBuilder& requestBuilder,
        const RecordingActionJobPayload& payload) const
    {
        if (payload.type == RecordingActionType::Move) {
            return requestBuilder.buildMoveRequest(config_, payload);
        }

        return requestBuilder.buildRenameRequest(config_, payload);
    }

    RestfulApiRecordingActionBackendConfig config_;
    IHttpClient& httpClient_;
};
