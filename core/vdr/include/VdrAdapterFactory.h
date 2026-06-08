#pragma once

#include <memory>

#include "IVdrAdapter.h"
#include "VdrConfig.h"

class VdrAdapterFactory {
public:
    static std::unique_ptr<IVdrAdapter> create(const VdrConfig& config);
};
