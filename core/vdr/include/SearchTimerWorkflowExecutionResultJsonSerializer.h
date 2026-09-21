#pragma once

#include "SearchTimerWorkflowExecutionResult.h"

#include <string>

class SearchTimerWorkflowExecutionResultJsonSerializer
{
public:
    std::string serialize(
        const SearchTimerWorkflowExecutionResult& result) const;
};
