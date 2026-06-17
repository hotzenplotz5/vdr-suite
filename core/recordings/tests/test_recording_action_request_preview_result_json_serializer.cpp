#include "RecordingActionRequestPreviewResultJsonSerializer.h"

#include <cassert>
#include <map>
#include <string>

int main()
{
    std::map<std::string, std::string> headers;
    headers["Accept"] = "application/json";
    headers["Content-Type"] = "application/json";

    RecordingActionRequestPreviewResult result =
        RecordingActionRequestPreviewResult::ok(
            RecordingActionType::Delete,
            "local-vdr",
            "Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec",
            "POST",
            "/recordings/delete.json",
            headers,
            "{\"file\":\"Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec\"}");

    RecordingActionRequestPreviewResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"success\":true") != std::string::npos);
    assert(json.find("\"type\":\"DELETE\"") != std::string::npos);
    assert(json.find("\"backendId\":\"local-vdr\"") != std::string::npos);
    assert(json.find("\"method\":\"POST\"") != std::string::npos);
    assert(json.find("\"url\":\"/recordings/delete.json\"") != std::string::npos);
    assert(json.find("\"Accept\":\"application/json\"") != std::string::npos);
    assert(json.find("\\\"file\\\"") != std::string::npos);
    assert(json.find("\"message\":\"recording action request preview built\"") != std::string::npos);

    return 0;
}
