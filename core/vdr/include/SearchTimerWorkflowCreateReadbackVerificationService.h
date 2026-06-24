#pragma once

#include "ISearchTimerDataSource.h"
#include "SearchTimerCreateResult.h"
#include "SearchTimerWorkflowBackendReadbackVerificationResult.h"

class SearchTimerWorkflowCreateReadbackVerificationService
{
public:
    SearchTimerWorkflowBackendReadbackVerificationResult verify(
        const SearchTimerCreateResult& createResult,
        const ISearchTimerDataSource* dataSource) const;
};
