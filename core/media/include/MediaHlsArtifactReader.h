#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct MediaHlsArtifact
{
    bool found = false;
    std::string reasonCode;
    std::string contentType;
    std::vector<unsigned char> bytes;
};

class MediaHlsArtifactReader
{
public:
    static constexpr std::size_t MaximumArtifactBytes = 16 * 1024 * 1024;

    explicit MediaHlsArtifactReader(std::string workspaceRoot);

    MediaHlsArtifact read(
        const std::string& workspaceId,
        const std::string& artifactName) const;

    static bool allowedArtifactName(const std::string& artifactName);

private:
    std::string workspaceRoot_;
};