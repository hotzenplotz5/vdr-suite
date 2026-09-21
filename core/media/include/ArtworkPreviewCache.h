#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <string>

// Owned by the existing artwork HTTP boundary. Receives only already-authorized
// image bytes, never client-supplied filesystem paths or remote URLs.
class ArtworkPreviewCache
{
public:
    explicit ArtworkPreviewCache(
        std::string directory = "/var/cache/vdr-suite/artwork-previews",
        std::string executable = "/usr/bin/ffmpeg");

    // Empty means use the original response. Successful output is image/jpeg.
    std::string home(const std::string& identity, const std::string& contentType,
                     const std::string& source) const;

private:
    std::string directory_;
    std::string executable_;
    mutable std::mutex mutex_;
    mutable std::map<std::string, std::chrono::steady_clock::time_point> failures_;
};
