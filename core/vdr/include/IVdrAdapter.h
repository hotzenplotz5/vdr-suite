#pragma once

#include "VdrChangeState.h"
#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrEventQuery.h"
#include "VdrRecording.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <vector>

class IVdrAdapter {
public:
    virtual ~IVdrAdapter() = default;

    virtual VdrStatus getStatus() const = 0;
    virtual std::vector<VdrEvent> getEvents() const = 0;
    virtual std::vector<VdrEvent> getEvents(const VdrEventQuery& query) const = 0;
    virtual std::vector<VdrChannel> getChannels() const = 0;
    virtual std::vector<VdrTimer> getTimers() const = 0;
    virtual std::vector<VdrRecording> getRecordings() const = 0;
    virtual VdrChangeState getChangeState() const = 0;
};
