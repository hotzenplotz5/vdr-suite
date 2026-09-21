#pragma once

#include <string>

struct Metadata
{
    int id = 0;

    int recordingId = 0;

    std::string mediaType;

    std::string title;
    std::string originalTitle;

    int year = 0;

    int seasonNumber = 0;
    int episodeNumber = 0;

    std::string genre;
    std::string description;

    std::string source;
    std::string externalId;
};
