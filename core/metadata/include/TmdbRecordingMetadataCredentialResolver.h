#pragma once

#include <string>

class TmdbRecordingMetadataCredentialResolver
{
public:
    static std::string resolveReadAccessToken(
        const std::string& backendId);

    static bool validBackendId(
        const std::string& backendId);
};
