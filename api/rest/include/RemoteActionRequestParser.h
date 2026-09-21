#pragma once

#include "RemoteActionDomain.h"

#include <string>
#include <vector>

struct RemoteActionParseResult
{
    bool validJson = false;
    RemoteActionRequest request;
    std::vector<std::string> errors;
};

class RemoteActionRequestParser
{
public:
    RemoteActionParseResult parse(const std::string& body) const;
};
