#pragma once

#include "IVdrAdapter.h"
#include "VdrEvent.h"
#include "VdrStatus.h"

#include <vector>

class MockVdrAdapter : public IVdrAdapter {
public:
    MockVdrAdapter();

    VdrStatus getStatus() const override;
    std::vector<VdrEvent> getEvents() const override;
};
