#pragma once

#include <string>

struct Recording
{
    int id = 0;

    std::string title;
    std::string subtitle;
    std::string description;
    std::string channel;
    std::string recordingPath;
    std::string recordingFormat;
};
