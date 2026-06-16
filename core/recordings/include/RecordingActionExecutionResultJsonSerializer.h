#pragma once

#include "RecordingActionExecutionResult.h"

#include <string>

class RecordingActionExecutionResultJsonSerializer
{
public:
    std::string serialize(
        const RecordingActionExecutionResult& result) const;
};
