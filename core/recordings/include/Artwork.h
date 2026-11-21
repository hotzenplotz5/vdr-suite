#pragma once

#include <string>

struct Artwork
{
    int id = 0;

    int metadataId = 0;
    int recordingId = 0;

    std::string posterPath;
    std::string fanartPath;
    std::string bannerPath;
    std::string thumbnailPath;

    std::string source;
};
