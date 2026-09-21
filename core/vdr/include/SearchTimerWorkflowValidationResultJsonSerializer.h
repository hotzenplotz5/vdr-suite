#pragma once

#include "SearchTimerWorkflowValidationResult.h"

#include <string>

class SearchTimerWorkflowValidationResultJsonSerializer
{
public:
    std::string serialize(
        const SearchTimerWorkflowValidationResult& result) const;
};
