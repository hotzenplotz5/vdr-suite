#pragma once

#include "IRecordingActionExecutor.h"

#include <memory>
#include <string>

struct RecordingActionDefaultExecutorResolutionResult
{
    bool resolved = false;

    bool usedDefaultExecutor = false;

    std::string backendId;

    std::shared_ptr<IRecordingActionExecutor> executor;

    std::string reason;
};
