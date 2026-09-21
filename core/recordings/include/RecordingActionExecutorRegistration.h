#pragma once

#include "IRecordingActionExecutor.h"

#include <memory>
#include <string>

struct RecordingActionExecutorRegistration
{
    std::string backendId;

    std::shared_ptr<IRecordingActionExecutor> executor;
};
