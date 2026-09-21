#pragma once

#include "ISeriesArtworkFallbackMaterializer.h"

#include <cstdint>
#include <string>
#include <vector>

struct FilesystemSeriesArtworkFallbackMaterializerConfig
{
    std::vector<std::string> allowedSourceRoots = {
        "/var/cache/vdr-suite/epg-artwork/incoming"
    };
    std::string cacheRoot =
        "/var/cache/vdr-suite/epg-artwork/external";
    std::uintmax_t maximumSourceBytes = 16U * 1024U * 1024U;
    int maximumDimension = 16384;
    std::uint64_t maximumPixels = 100000000U;
};

class FilesystemSeriesArtworkFallbackMaterializer final
    : public ISeriesArtworkFallbackMaterializer
{
public:
    explicit FilesystemSeriesArtworkFallbackMaterializer(
        FilesystemSeriesArtworkFallbackMaterializerConfig config = {});

    SeriesArtworkFallbackMaterializationResult materialize(
        const SeriesArtworkFallbackMaterializationRequest& request) override;

private:
    FilesystemSeriesArtworkFallbackMaterializerConfig config_;
};
