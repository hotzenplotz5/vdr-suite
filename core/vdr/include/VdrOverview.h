#pragma once

#include "VdrRecording.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

struct VdrOverview
{
    VdrStatus status;

    int totalChannels = 0;
    int radioChannels = 0;
    int encryptedChannels = 0;

    int totalEvents = 0;

    int totalTimers = 0;
    int activeTimers = 0;
    int recordingTimers = 0;

    bool hasNextTimer = false;
    VdrTimer nextTimer;

    int totalRecordings = 0;

    bool hasLatestRecording = false;
    VdrRecording latestRecording;
};
