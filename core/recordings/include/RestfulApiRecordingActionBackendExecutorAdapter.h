#pragma once

#include "HttpResponse.h"
#include "IHttpClient.h"
#include "IRecordingActionBackendExecutorAdapter.h"
#include "RecordingActionCapabilityContract.h"
#include "RestfulApiRecordingActionBackendConfig.h"
#include "RestfulApiRecordingActionRequestBuilder.h"

#include <algorithm>
#include <cctype>
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
        initializeResultIdentity(result, payload);

        if (!isSupportedAction(payload.type))
        {
            result.success = false;
            result.message =
                "restfulapi backend executor action not supported";
            result.errors.push_back(
                "unsupported recording action type for "
                "restfulapi backend executor");
            return result;
        }

        if (!validatePayload(payload, result))
        {
            return result;
        }

        if (!enforceReadOnlyPolicy(result))
        {
            return result;
        }

        if (!enforceExecutionPolicy(payload, result))
        {
            return result;
        }

        if (config_.apiMode ==
            RestfulApiRecordingActionApiMode::SafeMutation)
        {
            if (payload.type ==
                RecordingActionType::Move)
            {
                return executeSafeMove(payload);
            }

            if (payload.type ==
                RecordingActionType::Rename)
            {
                return executeSafeRename(payload);
            }

            if (payload.type ==
                RecordingActionType::Delete)
            {
                return executeSafeTrash(payload);
            }

            result.success = false;
            result.message =
                "restfulapi safe recording action "
                "workflow not implemented";
            result.errors.push_back(
                "safe recording action workflow "
                "is not implemented");
            return result;
        }

        RestfulApiRecordingActionRequestBuilder requestBuilder;

        const HttpRequest request =
            buildLegacyRequest(
                requestBuilder,
                payload);

        const HttpResponse response =
            httpClient_.execute(request);

        result.success =
            response.statusCode >= 200 &&
            response.statusCode < 300;

        result.message =
            result.success
                ? "restfulapi backend executor request accepted"
                : "restfulapi backend executor request failed";

        result.upstreamHttpStatus =
            response.statusCode;
        result.upstreamEndpoint =
            request.url;
        result.upstreamResponseBody =
            response.body;

        if (!result.success)
        {
            result.errors.push_back(
                "restfulapi backend returned HTTP status " +
                std::to_string(response.statusCode));

            if (!response.body.empty())
            {
                result.errors.push_back(
                    response.body);
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

    RecordingActionCapabilitySet capabilities() const
    {
        if (config_.apiMode ==
            RestfulApiRecordingActionApiMode::SafeMutation)
        {
            RecordingActionCapabilitySet capabilitySet;
            capabilitySet.add(
                "recording.action.move");
            capabilitySet.add(
                "recording.action.rename");
            capabilitySet.add(
                "recording.action.delete");
            return capabilitySet;
        }

        RecordingActionCapabilityContract contract;
        return contract.restfulApiDefaultCapabilities();
    }

    bool supportsAction(
        RecordingActionType type) const
    {
        RecordingActionCapabilityContract contract;

        const RecordingActionCapabilityCheckResult result =
            contract.check(
                type,
                capabilities());

        return result.supported;
    }

    const RestfulApiRecordingActionBackendConfig& config() const
    {
        return config_;
    }

private:
    static bool isSupportedAction(
        RecordingActionType type)
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
        const auto it =
            parameters.find(name);

        return
            it != parameters.end() &&
            !it->second.empty();
    }

    static std::string parameter(
        const std::map<std::string, std::string>& parameters,
        const std::string& name)
    {
        const auto it =
            parameters.find(name);

        if (it == parameters.end())
        {
            return "";
        }

        return it->second;
    }

    void initializeResultIdentity(
        RecordingActionExecutionResult& result,
        const RecordingActionJobPayload& payload) const
    {
        result.type = payload.type;
        result.backendId = config_.backendId;
        result.recordingId = payload.recordingId;

        result.backendNativeId =
            parameter(
                payload.parameters,
                "backendNativeId");

        result.recordingPath =
            parameter(
                payload.parameters,
                "recordingPath");
    }

    bool validatePayload(
        const RecordingActionJobPayload& payload,
        RecordingActionExecutionResult& result) const
    {
        if (payload.recordingId.empty())
        {
            result.success = false;
            result.message =
                "restfulapi backend executor payload invalid";
            result.errors.push_back(
                "recordingId is required");
            return false;
        }

        if (payload.type ==
                RecordingActionType::Move &&
            !hasParameter(
                payload.parameters,
                "targetPath"))
        {
            result.success = false;
            result.message =
                "restfulapi backend executor payload invalid";
            result.errors.push_back(
                "targetPath is required for move");
            return false;
        }

        if (payload.type ==
                RecordingActionType::Rename &&
            !hasParameter(
                payload.parameters,
                "newName"))
        {
            result.success = false;
            result.message =
                "restfulapi backend executor payload invalid";
            result.errors.push_back(
                "newName is required for rename");
            return false;
        }

        if (config_.apiMode ==
                RestfulApiRecordingActionApiMode::SafeMutation &&
            (payload.type ==
                 RecordingActionType::Move ||
             payload.type ==
                 RecordingActionType::Rename ||
             payload.type ==
                 RecordingActionType::Delete) &&
            !hasParameter(
                payload.parameters,
                "backendNativeId"))
        {
            result.success = false;
            result.message =
                "restfulapi backend executor payload invalid";
            result.errors.push_back(
                "backendNativeId is required for "
                "safe recording action");
            return false;
        }

        return true;
    }

    bool enforceReadOnlyPolicy(
        RecordingActionExecutionResult& result) const
    {
        if (!config_.readOnly)
        {
            return true;
        }

        result.success = false;
        result.message =
            "restfulapi backend executor backend is read-only";
        result.errors.push_back(
            "recording action execution is blocked by "
            "read-only backend config");
        return false;
    }

    bool enforceExecutionPolicy(
        const RecordingActionJobPayload& payload,
        RecordingActionExecutionResult& result) const
    {
        if (payload.dryRun)
        {
            return true;
        }

        if (config_.allowExecution)
        {
            return true;
        }

        result.success = false;
        result.message =
            "restfulapi backend executor execution disabled";
        result.errors.push_back(
            "real recording action execution is disabled "
            "by restfulapi backend config");
        return false;
    }

    static bool isHttpSuccess(
        const HttpResponse& response)
    {
        return
            response.statusCode >= 200 &&
            response.statusCode < 300;
    }

    static bool findJsonValue(
        const std::string& body,
        const std::string& key,
        std::size_t& position)
    {
        const std::string marker =
            "\"" + key + "\"";

        position =
            body.find(marker);

        if (position == std::string::npos)
        {
            return false;
        }

        position =
            body.find(
                ':',
                position + marker.size());

        if (position == std::string::npos)
        {
            return false;
        }

        ++position;

        while (position < body.size() &&
               std::isspace(
                   static_cast<unsigned char>(
                       body[position])) != 0)
        {
            ++position;
        }

        return position < body.size();
    }

    static bool parseJsonBool(
        const std::string& body,
        const std::string& key,
        bool& value)
    {
        std::size_t position = 0;

        if (!findJsonValue(
                body,
                key,
                position))
        {
            return false;
        }

        if (body.compare(
                position,
                4,
                "true") == 0)
        {
            value = true;
            return true;
        }

        if (body.compare(
                position,
                5,
                "false") == 0)
        {
            value = false;
            return true;
        }

        return false;
    }

    static bool parseJsonString(
        const std::string& body,
        const std::string& key,
        std::string& value)
    {
        std::size_t position = 0;

        if (!findJsonValue(
                body,
                key,
                position) ||
            body[position] != '"')
        {
            return false;
        }

        ++position;
        value.clear();

        bool escaped = false;

        for (; position < body.size(); ++position)
        {
            const char character =
                body[position];

            if (escaped)
            {
                value.push_back(character);
                escaped = false;
                continue;
            }

            if (character == '\\')
            {
                escaped = true;
                continue;
            }

            if (character == '"')
            {
                return true;
            }

            value.push_back(character);
        }

        return false;
    }

    static bool parseJsonIntegerText(
        const std::string& body,
        const std::string& key,
        std::string& value)
    {
        std::size_t position = 0;

        if (!findJsonValue(
                body,
                key,
                position))
        {
            return false;
        }

        if (body[position] == '"')
        {
            if (!parseJsonString(
                    body,
                    key,
                    value) ||
                value.empty())
            {
                return false;
            }

            return std::all_of(
                value.begin(),
                value.end(),
                [](char character)
                {
                    return std::isdigit(
                        static_cast<unsigned char>(
                            character)) != 0;
                });
        }

        const std::size_t start =
            position;

        while (position < body.size() &&
               std::isdigit(
                   static_cast<unsigned char>(
                       body[position])) != 0)
        {
            ++position;
        }

        if (position == start)
        {
            return false;
        }

        value =
            body.substr(
                start,
                position - start);

        return true;
    }

    RecordingActionExecutionResult stageFailure(
        const RecordingActionJobPayload& payload,
        const HttpRequest& request,
        const HttpResponse& response,
        const std::string& message,
        const std::string& detail) const
    {
        RecordingActionExecutionResult result;
        initializeResultIdentity(result, payload);

        result.success = false;
        result.message = message;
        result.upstreamHttpStatus =
            response.statusCode;
        result.upstreamEndpoint =
            request.url;
        result.upstreamResponseBody =
            response.body;

        if (!detail.empty())
        {
            result.errors.push_back(detail);
        }

        if (!response.body.empty() &&
            response.body != detail)
        {
            result.errors.push_back(
                response.body);
        }

        return result;
    }

    RecordingActionExecutionResult httpStageFailure(
        const RecordingActionJobPayload& payload,
        const HttpRequest& request,
        const HttpResponse& response,
        const std::string& workflow,
        const std::string& stage) const
    {
        return stageFailure(
            payload,
            request,
            response,
            "restfulapi safe " +
                workflow +
                " " +
                stage +
                " failed",
            "restfulapi safe " +
                workflow +
                " " +
                stage +
                " returned HTTP status " +
                std::to_string(
                    response.statusCode));
    }

    RecordingActionExecutionResult executeSafeMove(
        const RecordingActionJobPayload& payload)
    {
        RestfulApiRecordingActionRequestBuilder builder;

        const HttpRequest previewRequest =
            builder.buildSafeMovePreviewRequest(
                config_,
                payload);

        const HttpResponse previewResponse =
            httpClient_.execute(
                previewRequest);

        if (!isHttpSuccess(previewResponse))
        {
            return httpStageFailure(
                payload,
                previewRequest,
                previewResponse,
                "move",
                "preview");
        }

        bool executable = false;

        if (!parseJsonBool(
                previewResponse.body,
                "executable",
                executable))
        {
            return stageFailure(
                payload,
                previewRequest,
                previewResponse,
                "restfulapi safe move preview invalid",
                "safe move preview response is "
                "missing executable");
        }

        if (!executable)
        {
            return stageFailure(
                payload,
                previewRequest,
                previewResponse,
                "restfulapi safe move preview blocked",
                "safe move preview is not executable");
        }

        std::string recordingsState;
        std::string timersState;

        if (!parseJsonIntegerText(
                previewResponse.body,
                "revision_recordings_state",
                recordingsState) ||
            !parseJsonIntegerText(
                previewResponse.body,
                "revision_timers_state",
                timersState))
        {
            return stageFailure(
                payload,
                previewRequest,
                previewResponse,
                "restfulapi safe move preview invalid",
                "safe move preview response is "
                "missing revision state");
        }

        if (payload.dryRun)
        {
            RecordingActionExecutionResult result;
            initializeResultIdentity(result, payload);

            result.success = true;
            result.message =
                "restfulapi safe move preview accepted";
            result.upstreamHttpStatus =
                previewResponse.statusCode;
            result.upstreamEndpoint =
                previewRequest.url;
            result.upstreamResponseBody =
                previewResponse.body;

            return result;
        }

        const HttpRequest validateRequest =
            builder.buildSafeMoveValidateRequest(
                config_,
                payload,
                recordingsState,
                timersState);

        const HttpResponse validateResponse =
            httpClient_.execute(
                validateRequest);

        if (!isHttpSuccess(validateResponse))
        {
            return httpStageFailure(
                payload,
                validateRequest,
                validateResponse,
                "move",
                "validation");
        }

        std::string validationStatus;

        if (!parseJsonString(
                validateResponse.body,
                "status",
                validationStatus))
        {
            return stageFailure(
                payload,
                validateRequest,
                validateResponse,
                "restfulapi safe move validation invalid",
                "safe move validation response is "
                "missing status");
        }

        if (validationStatus != "ready")
        {
            return stageFailure(
                payload,
                validateRequest,
                validateResponse,
                "restfulapi safe move validation blocked",
                "safe move validation status is " +
                    validationStatus);
        }

        const HttpRequest executeRequest =
            builder.buildSafeMoveExecuteRequest(
                config_,
                payload,
                recordingsState,
                timersState);

        const HttpResponse executeResponse =
            httpClient_.execute(
                executeRequest);

        if (!isHttpSuccess(executeResponse))
        {
            return httpStageFailure(
                payload,
                executeRequest,
                executeResponse,
                "move",
                "execution");
        }

        std::string executionStatus;

        if (!parseJsonString(
                executeResponse.body,
                "status",
                executionStatus))
        {
            return stageFailure(
                payload,
                executeRequest,
                executeResponse,
                "restfulapi safe move execution invalid",
                "safe move execution response is "
                "missing status");
        }

        if (executionStatus != "moved" &&
            executionStatus != "already-moved")
        {
            return stageFailure(
                payload,
                executeRequest,
                executeResponse,
                "restfulapi safe move execution failed",
                "unexpected safe move execution status " +
                    executionStatus);
        }

        RecordingActionExecutionResult result;
        initializeResultIdentity(result, payload);

        result.success = true;
        result.message =
            executionStatus == "already-moved"
                ? "restfulapi safe move already completed"
                : "restfulapi safe move request accepted";

        result.upstreamHttpStatus =
            executeResponse.statusCode;
        result.upstreamEndpoint =
            executeRequest.url;
        result.upstreamResponseBody =
            executeResponse.body;

        return result;
    }

    RecordingActionExecutionResult executeSafeRename(
        const RecordingActionJobPayload& payload)
    {
        RestfulApiRecordingActionRequestBuilder builder;

        const HttpRequest previewRequest =
            builder.buildSafeRenamePreviewRequest(
                config_,
                payload);

        const HttpResponse previewResponse =
            httpClient_.execute(
                previewRequest);

        if (!isHttpSuccess(previewResponse))
        {
            return httpStageFailure(
                payload,
                previewRequest,
                previewResponse,
                "rename",
                "preview");
        }

        bool executable = false;

        if (!parseJsonBool(
                previewResponse.body,
                "executable",
                executable))
        {
            return stageFailure(
                payload,
                previewRequest,
                previewResponse,
                "restfulapi safe rename preview invalid",
                "safe rename preview response is "
                "missing executable");
        }

        if (!executable)
        {
            return stageFailure(
                payload,
                previewRequest,
                previewResponse,
                "restfulapi safe rename preview blocked",
                "safe rename preview is not executable");
        }

        std::string recordingsState;
        std::string timersState;

        if (!parseJsonIntegerText(
                previewResponse.body,
                "revision_recordings_state",
                recordingsState) ||
            !parseJsonIntegerText(
                previewResponse.body,
                "revision_timers_state",
                timersState))
        {
            return stageFailure(
                payload,
                previewRequest,
                previewResponse,
                "restfulapi safe rename preview invalid",
                "safe rename preview response is "
                "missing revision state");
        }

        if (payload.dryRun)
        {
            RecordingActionExecutionResult result;
            initializeResultIdentity(result, payload);

            result.success = true;
            result.message =
                "restfulapi safe rename preview accepted";
            result.upstreamHttpStatus =
                previewResponse.statusCode;
            result.upstreamEndpoint =
                previewRequest.url;
            result.upstreamResponseBody =
                previewResponse.body;

            return result;
        }

        const HttpRequest validateRequest =
            builder.buildSafeRenameValidateRequest(
                config_,
                payload,
                recordingsState,
                timersState);

        const HttpResponse validateResponse =
            httpClient_.execute(
                validateRequest);

        if (!isHttpSuccess(validateResponse))
        {
            return httpStageFailure(
                payload,
                validateRequest,
                validateResponse,
                "rename",
                "validation");
        }

        std::string validationStatus;

        if (!parseJsonString(
                validateResponse.body,
                "status",
                validationStatus))
        {
            return stageFailure(
                payload,
                validateRequest,
                validateResponse,
                "restfulapi safe rename validation invalid",
                "safe rename validation response is "
                "missing status");
        }

        if (validationStatus != "ready")
        {
            return stageFailure(
                payload,
                validateRequest,
                validateResponse,
                "restfulapi safe rename validation blocked",
                "safe rename validation status is " +
                    validationStatus);
        }

        const HttpRequest executeRequest =
            builder.buildSafeRenameExecuteRequest(
                config_,
                payload,
                recordingsState,
                timersState);

        const HttpResponse executeResponse =
            httpClient_.execute(
                executeRequest);

        if (!isHttpSuccess(executeResponse))
        {
            return httpStageFailure(
                payload,
                executeRequest,
                executeResponse,
                "rename",
                "execution");
        }

        std::string executionStatus;

        if (!parseJsonString(
                executeResponse.body,
                "status",
                executionStatus))
        {
            return stageFailure(
                payload,
                executeRequest,
                executeResponse,
                "restfulapi safe rename execution invalid",
                "safe rename execution response is "
                "missing status");
        }

        if (executionStatus != "renamed" &&
            executionStatus != "already-renamed")
        {
            return stageFailure(
                payload,
                executeRequest,
                executeResponse,
                "restfulapi safe rename execution failed",
                "unexpected safe rename execution status " +
                    executionStatus);
        }

        RecordingActionExecutionResult result;
        initializeResultIdentity(result, payload);

        result.success = true;
        result.message =
            executionStatus == "already-renamed"
                ? "restfulapi safe rename already completed"
                : "restfulapi safe rename request accepted";

        result.upstreamHttpStatus =
            executeResponse.statusCode;
        result.upstreamEndpoint =
            executeRequest.url;
        result.upstreamResponseBody =
            executeResponse.body;

        return result;
    }

    RecordingActionExecutionResult executeSafeTrash(
        const RecordingActionJobPayload& payload)
    {
        RestfulApiRecordingActionRequestBuilder builder;

        const HttpRequest previewRequest =
            builder.buildSafeTrashPreviewRequest(
                config_,
                payload);

        const HttpResponse previewResponse =
            httpClient_.execute(
                previewRequest);

        if (!isHttpSuccess(previewResponse))
        {
            return httpStageFailure(
                payload,
                previewRequest,
                previewResponse,
                "trash",
                "preview");
        }

        bool executable = false;

        if (!parseJsonBool(
                previewResponse.body,
                "executable",
                executable))
        {
            return stageFailure(
                payload,
                previewRequest,
                previewResponse,
                "restfulapi safe trash preview invalid",
                "safe trash preview response is "
                "missing executable");
        }

        if (!executable)
        {
            return stageFailure(
                payload,
                previewRequest,
                previewResponse,
                "restfulapi safe trash preview blocked",
                "safe trash preview is not executable");
        }

        std::string recordingsState;
        std::string timersState;

        if (!parseJsonIntegerText(
                previewResponse.body,
                "revision_recordings_state",
                recordingsState) ||
            !parseJsonIntegerText(
                previewResponse.body,
                "revision_timers_state",
                timersState))
        {
            return stageFailure(
                payload,
                previewRequest,
                previewResponse,
                "restfulapi safe trash preview invalid",
                "safe trash preview response is "
                "missing revision state");
        }

        if (payload.dryRun)
        {
            RecordingActionExecutionResult result;
            initializeResultIdentity(
                result,
                payload);

            result.success = true;
            result.message =
                "restfulapi safe trash preview accepted";

            result.upstreamHttpStatus =
                previewResponse.statusCode;

            result.upstreamEndpoint =
                previewRequest.url;

            result.upstreamResponseBody =
                previewResponse.body;

            return result;
        }

        const HttpRequest validateRequest =
            builder.buildSafeTrashValidateRequest(
                config_,
                payload,
                recordingsState,
                timersState);

        const HttpResponse validateResponse =
            httpClient_.execute(
                validateRequest);

        if (!isHttpSuccess(validateResponse))
        {
            return httpStageFailure(
                payload,
                validateRequest,
                validateResponse,
                "trash",
                "validation");
        }

        std::string validationStatus;

        if (!parseJsonString(
                validateResponse.body,
                "status",
                validationStatus))
        {
            return stageFailure(
                payload,
                validateRequest,
                validateResponse,
                "restfulapi safe trash validation invalid",
                "safe trash validation response is "
                "missing status");
        }

        if (validationStatus != "ready")
        {
            return stageFailure(
                payload,
                validateRequest,
                validateResponse,
                "restfulapi safe trash validation blocked",
                "safe trash validation status is " +
                    validationStatus);
        }

        const HttpRequest executeRequest =
            builder.buildSafeTrashExecuteRequest(
                config_,
                payload,
                recordingsState,
                timersState);

        const HttpResponse executeResponse =
            httpClient_.execute(
                executeRequest);

        if (!isHttpSuccess(executeResponse))
        {
            return httpStageFailure(
                payload,
                executeRequest,
                executeResponse,
                "trash",
                "execution");
        }

        std::string executionStatus;

        if (!parseJsonString(
                executeResponse.body,
                "status",
                executionStatus))
        {
            return stageFailure(
                payload,
                executeRequest,
                executeResponse,
                "restfulapi safe trash execution invalid",
                "safe trash execution response is "
                "missing status");
        }

        if (executionStatus != "trashed" &&
            executionStatus != "already-trashed")
        {
            return stageFailure(
                payload,
                executeRequest,
                executeResponse,
                "restfulapi safe trash execution failed",
                "unexpected safe trash execution status " +
                    executionStatus);
        }

        RecordingActionExecutionResult result;
        initializeResultIdentity(
            result,
            payload);

        result.success = true;

        result.message =
            executionStatus == "already-trashed"
                ? "restfulapi safe trash already completed"
                : "restfulapi safe trash request accepted";

        result.upstreamHttpStatus =
            executeResponse.statusCode;

        result.upstreamEndpoint =
            executeRequest.url;

        result.upstreamResponseBody =
            executeResponse.body;

        return result;
    }

    HttpRequest buildLegacyRequest(
        RestfulApiRecordingActionRequestBuilder& requestBuilder,
        const RecordingActionJobPayload& payload) const
    {
        if (payload.type ==
            RecordingActionType::Move)
        {
            return requestBuilder.buildMoveRequest(
                config_,
                payload);
        }

        if (payload.type ==
            RecordingActionType::Rename)
        {
            return requestBuilder.buildRenameRequest(
                config_,
                payload);
        }

        return requestBuilder.buildDeleteRequest(
            config_,
            payload);
    }

    RestfulApiRecordingActionBackendConfig config_;
    IHttpClient& httpClient_;
};
