#pragma once

#include "VdrStatus.h"

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

    int totalRecordings = 0;
};
