#pragma once

#include <string>
#include <vector>

struct SearchTimerWorkflowProductionExecutorHardeningRequirement
{
    std::string id;
    std::string title;
    bool mandatory = true;
    bool satisfied = false;
    std::string status;
    std::string detail;
};

struct SearchTimerWorkflowProductionExecutorHardeningPlanResult
{
    bool readyForProductionExecution = false;
    std::vector<SearchTimerWorkflowProductionExecutorHardeningRequirement> requirements;
    std::vector<std::string> blockers;

    bool hasBlockers() const
    {
        return !blockers.empty();
    }
};

class SearchTimerWorkflowProductionExecutorHardeningPlan
{
public:
    SearchTimerWorkflowProductionExecutorHardeningPlanResult build() const;
};
