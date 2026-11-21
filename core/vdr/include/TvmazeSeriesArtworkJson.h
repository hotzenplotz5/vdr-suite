#pragma once

#include <cstddef>
#include <string>

struct TvmazeSeriesImage
{
    int imageId = 0;
    std::string url;
    int width = 0;
    int height = 0;
    bool background = false;
    bool main = false;
};

bool parseTvmazeShowLocation(
    const std::string& location,
    int& showId);

bool parseTvmazeSeriesImage(
    const std::string& body,
    std::size_t maximumBytes,
    TvmazeSeriesImage& image);
