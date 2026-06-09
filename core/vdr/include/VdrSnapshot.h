#ifndef VDR_SNAPSHOT_H
#define VDR_SNAPSHOT_H

#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <string>
#include <vector>

struct VdrSnapshot {
    std::string backendId = "default";
    VdrStatus status;
    std::vector<VdrRecording> recordings;
    std::vector<VdrTimer> timers;
    std::vector<VdrChannel> channels;
    std::vector<VdrEvent> events;
};

#endif
