#pragma once

#include <string>

enum class RestfulApiRecordingActionApiMode
{
    Legacy,
    SafeMutation
};

struct RestfulApiRecordingActionBackendConfig
{
    std::string backendId;
    std::string host;
    int port = 0;
    std::string basePath;
    std::string videoDirectory = "/srv/vdr/video";
    bool videoDirectoryExplicit = false;
    RestfulApiRecordingActionApiMode apiMode =
        RestfulApiRecordingActionApiMode::Legacy;
    bool allowExecution = false;
    bool readOnly = false;
};
