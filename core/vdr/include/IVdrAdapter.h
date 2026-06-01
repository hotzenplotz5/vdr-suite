#pragma once

#include "VdrEvent.h"
#include "VdrStatus.h"

#include <vector>

class IVdrAdapter {
public:
    virtual ~IVdrAdapter() = default;

    virtual VdrStatus getStatus() const = 0;
    virtual std::vector<VdrEvent> getEvents() const = 0;
};
