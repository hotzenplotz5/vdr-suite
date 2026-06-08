#pragma once

#include "RecordingAction.h"

#include <string>

std::string toString(
    RecordingActionType type);

RecordingActionType fromString(
    const std::string& value);
