#pragma once

#include "IVdrTimerActionExecutorAdapter.h"

#include <memory>
#include <string>

struct VdrTimerActionExecutorAdapterLookupResult
{
    bool found = false;

    std::string backendId;

    std::shared_ptr<IVdrTimerActionExecutorAdapter> adapter;

    std::string message;
};
