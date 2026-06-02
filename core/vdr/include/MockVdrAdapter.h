#pragma once

#include "IVdrAdapter.h"
#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrStatus.h"

#include <vector>

class MockVdrAdapter : public IVdrAdapter {
public:
    MockVdrAdapter();

    VdrStatus getStatus() const override;
    std::vector<VdrEvent> getEvents() const override;
    std::vector<VdrChannel> getChannels() const override;
};
