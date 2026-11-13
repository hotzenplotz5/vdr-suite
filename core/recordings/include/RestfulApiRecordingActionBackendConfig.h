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
    // Supplied by the matching backend configuration or VDR discovery.
    std::string videoDirectory;
    bool videoDirectoryExplicit = false;
    RestfulApiRecordingActionApiMode apiMode =
        RestfulApiRecordingActionApiMode::Legacy;
    bool allowExecution = false;
    bool readOnly = false;
};
