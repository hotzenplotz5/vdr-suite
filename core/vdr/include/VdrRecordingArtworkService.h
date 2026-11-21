#pragma once

#include <chrono>
#include <cstddef>
#include <map>
#include <mutex>
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
    struct ArtworkLookupIndex
    {
        bool initialized = false;
        std::chrono::steady_clock::time_point rebuiltAt{};
        std::map<std::string, std::string> referencesByAssetId;
    };

    bool resolveArtworkReference(
        const std::string& backendId,
        const std::string& assetId,
        std::string& reference) const;

    VdrRecordingCacheRepository& repository_;
    std::map<std::string, std::string> rootsByBackend_;
    std::size_t maximumFileSizeBytes_;
    mutable std::mutex artworkIndexMutex_;
    mutable std::map<std::string, ArtworkLookupIndex> artworkIndexes_;
};
