#pragma once

#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <string>
#include <vector>

class VdrSnapshotReadJsonSerializer
{
public:
    std::string serializeStatus(
        const VdrStatus& status) const;

    std::string serializeRecordings(
        const std::vector<VdrRecording>& recordings) const;

    std::string serializeTimers(
        const std::vector<VdrTimer>& timers) const;

    std::string serializeChannels(
        const std::vector<VdrChannel>& channels) const;

    std::string serializeEvents(
        const std::vector<VdrEvent>& events) const;

    std::string serializeHealth(
        bool snapshotAvailable,
        const VdrStatus& status,
        std::size_t channelCount,
        std::size_t eventCount,
        std::size_t timerCount,
        std::size_t recordingCount) const;

    std::string serializeSnapshotSummary(
        bool snapshotAvailable,
        std::size_t channelCount,
        std::size_t eventCount,
        std::size_t timerCount,
        std::size_t recordingCount) const;
};
