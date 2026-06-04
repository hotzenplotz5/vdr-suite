#pragma once

#include "IVdrAdapter.h"

class MockVdrAdapter : public IVdrAdapter {
public:
    VdrStatus getStatus() const override;
    std::vector<VdrEvent> getEvents() const override;
    std::vector<VdrChannel> getChannels() const override;
    std::vector<VdrTimer> getTimers() const override;
    std::vector<VdrRecording> getRecordings() const override;
    VdrChangeState getChangeState() const override;
};