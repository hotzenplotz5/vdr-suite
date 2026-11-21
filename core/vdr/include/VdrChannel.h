#pragma once

#include <string>

struct VdrChannel
{
    std::string id;
    int number;
    std::string name;
    std::string provider;
    std::string group;
    bool radio;
    bool encrypted;
    bool enabled;
};
