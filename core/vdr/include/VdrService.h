#pragma once

#include "IVdrAdapter.h"
#include "VdrChangeState.h"
#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <vector>

class VdrService
{
public:
    explicit VdrService(IVdrAdapter& adapter);

    VdrStatus getStatus() const;
    std::vector<VdrChannel> getChannels() const;
    std::vector<VdrEvent> getEvents() const;
    std::vector<VdrTimer> getTimers() const;
    std::vector<VdrRecording> getRecordings() const;
    VdrChangeState getChangeState() const;

private:
    IVdrAdapter& adapter_;
};
