#pragma once

#include <string>

enum class EpgArtworkReferenceOrigin
{
    Unknown,
    PrimaryMetadata,
    ExternalFallback
};

struct EpgArtworkReference
{
    std::string backendId;
    std::string channelId;
    std::string eventId;

    std::string provider;
    EpgArtworkReferenceOrigin origin = EpgArtworkReferenceOrigin::Unknown;
    std::string path;
    int width = 0;
    int height = 0;
    long long resolvedAt = 0;

    bool valid() const
    {
        return !backendId.empty() &&
            !channelId.empty() &&
            !eventId.empty() &&
            !provider.empty() &&
            !path.empty() &&
            width > 0 &&
            height > 0;
    }
};
