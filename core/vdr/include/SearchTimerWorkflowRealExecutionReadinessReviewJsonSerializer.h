#pragma once

#include "SearchTimerWorkflowRealExecutionReadinessReview.h"

#include <string>

class SearchTimerWorkflowRealExecutionReadinessReviewJsonSerializer
{
public:
    std::string serialize(
        const SearchTimerWorkflowRealExecutionReadinessReviewResult& result) const;
};
