#pragma once

#include <string>

struct EpgArtworkReference
{
    std::string backendId;
    std::string channelId;
    std::string eventId;

    std::string provider;
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
