#pragma once

#include "RecordingActionRequest.h"

#include <string>

class RecordingActionValidationRequestParser
{
public:
    RecordingActionRequest parse(
        const std::string& body) const;
};
