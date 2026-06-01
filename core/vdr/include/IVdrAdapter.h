
#pragma once

#include "VdrStatus.h"

class IVdrAdapter {
public:
    virtual ~IVdrAdapter() = default;

    virtual VdrStatus getStatus() const = 0;
};
