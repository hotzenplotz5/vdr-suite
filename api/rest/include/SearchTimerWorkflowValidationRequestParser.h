#pragma once

#include "SearchTimerWorkflowRequest.h"

#include <string>

class SearchTimerWorkflowValidationRequestParser
{
public:
    SearchTimerWorkflowRequest parse(
        const std::string& body) const;
};
