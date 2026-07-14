#pragma once

#include "HttpRequest.h"
#include "RecordingActionJobPayload.h"
#include "RestfulApiRecordingActionBackendConfig.h"

#include <map>
#include <string>

class RestfulApiRecordingTrashRequestBuilder
{
public:
    HttpRequest buildPreviewRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url = buildUrl(
            config.basePath,
            "/recordings/trash/preview.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body = buildPreviewBody(payload);
        return request;
    }

    HttpRequest buildValidateRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload,
        long long recordingsState,
        long long timersState) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url = buildUrl(
            config.basePath,
            "/recordings/trash/validate.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body = buildRevisionBody(
            payload,
            recordingsState,
            timersState);
        return request;
    }

    HttpRequest buildExecuteRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload,
        long long recordingsState,
        long long timersState) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url = buildUrl(
            config.basePath,
            "/recordings/trash.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body = buildRevisionBody(
            payload,
            recordingsState,
            timersState);
        return request;
    }

private:
    static std::string findParameter(
        const std::map<std::string, std::string>& parameters,
        const std::string& name)
    {
        const auto it = parameters.find(name);
        return it == parameters.end() ? "" : it->second;
    }

    static std::string recordingPath(
        const RecordingActionJobPayload& payload)
    {
        const std::string backendNativeId =
            findParameter(payload.parameters, "backendNativeId");

        if (!backendNativeId.empty())
        {
            return backendNativeId;
        }

        const std::string path =
            findParameter(payload.parameters, "recordingPath");

        if (!path.empty())
        {
            return path;
        }

        return payload.recordingId;
    }

    static std::string buildUrl(
        const std::string& basePath,
        const std::string& endpoint)
    {
        if (basePath.empty())
        {
            return endpoint;
        }

        if (basePath.back() == '/' && endpoint.front() == '/')
        {
            return basePath.substr(0, basePath.size() - 1) + endpoint;
        }

        if (basePath.back() != '/' && endpoint.front() != '/')
        {
            return basePath + "/" + endpoint;
        }

        return basePath + endpoint;
    }

    static std::string jsonQuote(const std::string& value)
    {
        std::string quoted = "\"";

        for (char character : value)
        {
            if (character == '"' || character == '\\')
            {
                quoted += '\\';
            }

            quoted += character;
        }

        quoted += "\"";
        return quoted;
    }

    static std::string buildPreviewBody(
        const RecordingActionJobPayload& payload)
    {
        return "{\"file\":" + jsonQuote(recordingPath(payload)) + "}";
    }

    static std::string buildRevisionBody(
        const RecordingActionJobPayload& payload,
        long long recordingsState,
        long long timersState)
    {
        std::string body = "{";
        body += "\"file\":" + jsonQuote(recordingPath(payload));
        body += ",\"revision_recordings_state\":" +
            jsonQuote(std::to_string(recordingsState));
        body += ",\"revision_timers_state\":" +
            jsonQuote(std::to_string(timersState));
        body += "}";
        return body;
    }
};
