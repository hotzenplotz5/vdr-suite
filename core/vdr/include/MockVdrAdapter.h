#pragma once

#include "IVdrAdapter.h"
#include "VdrStatus.h"

class MockVdrAdapter : public IVdrAdapter {
public:
    MockVdrAdapter();

    VdrStatus getStatus() const override;
};
