#pragma once

#include <cstddef>
#include <string>
#include <vector>

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
        std::vector<std::string> roots,
        std::size_t maximumFileSizeBytes = 16U * 1024U * 1024U);

    bool handlesPath(
        const std::string& requestPath) const;

    // Resolve only opaque Suite artwork requests backed by cached Recording
    // metadata and files contained by an explicitly configured local root.
    VdrRecordingArtworkAsset loadPath(
        const std::string& requestPath) const;

private:
    VdrRecordingCacheRepository& repository_;
    std::vector<std::string> roots_;
    std::size_t maximumFileSizeBytes_;
};
