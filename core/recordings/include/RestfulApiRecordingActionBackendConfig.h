#pragma once

#include <string>

struct RestfulApiRecordingActionBackendConfig
{
    std::string backendId;
    std::string host;
    int port = 0;
    std::string basePath;
};
