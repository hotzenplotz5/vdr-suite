#pragma once

#include "HttpRequest.h"
#include "RecordingAction.h"
#include "RecordingActionJobPayload.h"
#include "RestfulApiRecordingActionBackendConfig.h"

#include <map>
#include <string>

class RestfulApiRecordingActionRequestBuilder
{
public:
    HttpRequest buildMoveRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url = buildUrl(config.basePath, "/recordings/actions/move");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body = buildMoveBody(payload);
        return request;
    }

    HttpRequest buildRenameRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url = buildUrl(config.basePath, "/recordings/actions/rename");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body = buildRenameBody(payload);
        return request;
    }

private:
    static std::string buildUrl(
        const std::string& basePath,
        const std::string& endpoint)
    {
        if (basePath.empty()) {
            return endpoint;
        }

        if (basePath.back() == '/' && endpoint.front() == '/') {
            return basePath.substr(0, basePath.size() - 1) + endpoint;
        }

        if (basePath.back() != '/' && endpoint.front() != '/') {
            return basePath + "/" + endpoint;
        }

        return basePath + endpoint;
    }

    static std::string findParameter(
        const std::map<std::string, std::string>& parameters,
        const std::string& name)
    {
        const auto it = parameters.find(name);

        if (it == parameters.end()) {
            return "";
        }

        return it->second;
    }

    static std::string jsonQuote(const std::string& value)
    {
        std::string quoted = "\"";

        for (char c : value) {
            if (c == '"' || c == '\\') {
                quoted += '\\';
            }

            quoted += c;
        }

        quoted += "\"";
        return quoted;
    }

    static std::string buildMoveBody(
        const RecordingActionJobPayload& payload)
    {
        const std::string targetPath =
            findParameter(payload.parameters, "targetPath");

        std::string body = "{";
        body += "\"recordingId\":" + jsonQuote(payload.recordingId);
        body += ",\"dryRun\":";
        body += payload.dryRun ? "true" : "false";
        body += ",\"targetPath\":" + jsonQuote(targetPath);
        body += "}";

        return body;
    }

    static std::string buildRenameBody(
        const RecordingActionJobPayload& payload)
    {
        const std::string newName =
            findParameter(payload.parameters, "newName");

        std::string body = "{";
        body += "\"recordingId\":" + jsonQuote(payload.recordingId);
        body += ",\"dryRun\":";
        body += payload.dryRun ? "true" : "false";
        body += ",\"newName\":" + jsonQuote(newName);
        body += "}";

        return body;
    }
};
