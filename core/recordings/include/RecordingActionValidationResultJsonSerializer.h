#pragma once

#include "RecordingActionValidationResult.h"

#include <string>

class RecordingActionValidationResultJsonSerializer
{
public:
    std::string serialize(
        const RecordingActionValidationResult& result) const;
};
