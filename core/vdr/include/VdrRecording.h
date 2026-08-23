#pragma once

#include "Person.h"
#include "VdrRecordingMetadata.h"

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
    bool recordingDurationKnown = false;
    long long sizeMb = 0;

    VdrRecordingMetadata metadata;
    PersonCollection persons = PersonCollection::createEmpty();
};