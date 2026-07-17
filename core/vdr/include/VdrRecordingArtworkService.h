#pragma once

#include <cstddef>
#include <map>
#include <string>

class VdrRecordingCacheRepository;

struct VdrRecordingArtworkAsset
{
    int statusCode = 404;
    std::string contentType;
    std::string content;

    bool found() const
    {
        return statusCode == 200;
    }
};

class VdrRecordingArtworkService
{
public:
    VdrRecordingArtworkService(
        VdrRecordingCacheRepository& repository,
        std::map<std::string, std::string> rootsByBackend,
        std::size_t maximumFileSizeBytes = 16U * 1024U * 1024U);

    bool handlesPath(
        const std::string& requestPath) const;

    // Resolve only opaque Suite artwork requests backed by cached Recording
    // metadata and the explicit local root assigned to that Recording backend.
    VdrRecordingArtworkAsset loadPath(
        const std::string& requestPath) const;

private:
    VdrRecordingCacheRepository& repository_;
    std::map<std::string, std::string> rootsByBackend_;
    std::size_t maximumFileSizeBytes_;
};
