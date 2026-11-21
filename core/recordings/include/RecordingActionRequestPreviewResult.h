#pragma once

#include "RecordingAction.h"

#include <map>
#include <string>
#include <vector>

struct RecordingActionRequestPreviewResult
{
    bool success = false;
    RecordingActionType type = RecordingActionType::Unknown;
    std::string backendId;
    std::string recordingId;
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    static RecordingActionRequestPreviewResult ok(
        RecordingActionType type,
        const std::string& backendId,
        const std::string& recordingId,
        const std::string& method,
        const std::string& url,
        const std::map<std::string, std::string>& headers,
        const std::string& body)
    {
        RecordingActionRequestPreviewResult result;
        result.success = true;
        result.type = type;
        result.backendId = backendId;
        result.recordingId = recordingId;
        result.method = method;
        result.url = url;
        result.headers = headers;
        result.body = body;
        result.message = "recording action request preview built";
        return result;
    }

    static RecordingActionRequestPreviewResult failed(
        RecordingActionType type,
        const std::string& backendId,
        const std::string& recordingId,
        const std::string& message,
        const std::vector<std::string>& errors)
    {
        RecordingActionRequestPreviewResult result;
        result.success = false;
        result.type = type;
        result.backendId = backendId;
        result.recordingId = recordingId;
        result.message = message;
        result.errors = errors;
        return result;
    }
};
