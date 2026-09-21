#pragma once

#include <cstddef>
#include <string>

struct TmdbSeriesBackdrop
{
    std::string filePath;
    std::string language;
    bool languageIsNull = false;
    int width = 0;
    int height = 0;
    double voteAverage = 0.0;
};

bool parseTmdbFindSeriesId(
    const std::string& body,
    std::size_t maximumBytes,
    int& seriesId);

bool parseTmdbSeriesBackdrop(
    const std::string& body,
    std::size_t maximumBytes,
    const std::string& preferredLanguage,
    TmdbSeriesBackdrop& backdrop);
