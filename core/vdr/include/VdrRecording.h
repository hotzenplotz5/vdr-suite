#pragma once

#include "Person.h"

#include <string>

struct VdrRecording
{
    std::string id;
    std::string backendId;

    std::string title;
    std::string path;
    std::string backendNativeId;

    std::string startTime;

    int durationSeconds = 0;
    long long sizeMb = 0;

    PersonCollection persons = PersonCollection::createEmpty();
};
