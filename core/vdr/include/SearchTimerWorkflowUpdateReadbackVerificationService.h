#pragma once

#include "ISearchTimerDataSource.h"
#include "SearchTimerUpdateResult.h"
#include "SearchTimerWorkflowBackendReadbackVerificationResult.h"

class SearchTimerWorkflowUpdateReadbackVerificationService
{
public:
    SearchTimerWorkflowBackendReadbackVerificationResult verify(
        const SearchTimerUpdateResult& updateResult,
        const ISearchTimerDataSource* dataSource) const;
};
