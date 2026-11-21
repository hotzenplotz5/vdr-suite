#pragma once

#include "RecordingActionRequest.h"

#include <string>

class RecordingActionValidationRequestParser
{
public:
    RecordingActionRequest parse(
        const std::string& body) const;

    // Public-v1 accepts opaque IDs, never client-supplied native selectors.
    bool parsePublic(
        const std::string& body, RecordingActionRequest& request) const;
};
