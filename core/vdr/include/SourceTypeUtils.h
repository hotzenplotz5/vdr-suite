#pragma once

#include "SourceType.h"

#include <string>

std::string toString(SourceType type);

SourceType sourceTypeFromString(const std::string& value);
