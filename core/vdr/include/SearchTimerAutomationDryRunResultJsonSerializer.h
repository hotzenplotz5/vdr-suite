#pragma once

#include "SearchTimerAutomationDryRunResult.h"

#include <string>

class SearchTimerAutomationDryRunResultJsonSerializer
{
public:
    std::string serialize(
        const SearchTimerAutomationDryRunResult& result) const;
};
