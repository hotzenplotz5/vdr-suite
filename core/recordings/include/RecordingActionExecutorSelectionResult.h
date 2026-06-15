#pragma once

#include "IRecordingActionExecutor.h"

#include <memory>
#include <string>

struct RecordingActionExecutorSelectionResult
{
    bool selected = false;

    std::string backendId;

    std::shared_ptr<IRecordingActionExecutor> executor;

    std::string reason;
};
