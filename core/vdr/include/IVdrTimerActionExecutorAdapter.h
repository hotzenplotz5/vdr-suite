#pragma once

#include "IVdrTimerActionExecutor.h"

#include <string>

class IVdrTimerActionExecutorAdapter
{
public:
    virtual ~IVdrTimerActionExecutorAdapter() = default;

    virtual std::string backendId() const = 0;

    virtual IVdrTimerActionExecutor& executor() = 0;
};
