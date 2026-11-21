#pragma once

#include <string>

struct EpgSeriesArtworkFallbackAsset
{
    std::string contentType;
    std::string content;
    int width = 0;
    int height = 0;

    bool valid() const
    {
        return (contentType == "image/png" || contentType == "image/jpeg") &&
            !content.empty() && width > 0 && height > 0;
    }
};

class IEpgSeriesArtworkFallbackDeliveryProvider
{
public:
    virtual ~IEpgSeriesArtworkFallbackDeliveryProvider() = default;

    virtual EpgSeriesArtworkFallbackAsset loadSeriesArtworkFallback(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const = 0;
};
