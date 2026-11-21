#pragma once

#include "SearchTimerWorkflowRequest.h"
#include "SearchTimerWorkflowValidationResult.h"

class SearchTimerWorkflowValidationService
{
public:
    SearchTimerWorkflowValidationResult validate(
        const SearchTimerWorkflowRequest& request) const;
};
