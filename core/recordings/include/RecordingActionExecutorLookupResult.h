#pragma once

#include "IRecordingActionExecutor.h"

#include <memory>
#include <string>

struct RecordingActionExecutorLookupResult
{
    bool found = false;

    std::string backendId;

    std::shared_ptr<IRecordingActionExecutor> executor;

    std::string message;
};
