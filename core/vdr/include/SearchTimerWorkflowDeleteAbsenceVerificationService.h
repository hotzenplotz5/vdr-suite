#pragma once

#include "ISearchTimerDataSource.h"
#include "SearchTimerDeleteResult.h"
#include "SearchTimerWorkflowBackendReadbackVerificationResult.h"

class SearchTimerWorkflowDeleteAbsenceVerificationService
{
public:
    SearchTimerWorkflowBackendReadbackVerificationResult verify(
        const SearchTimerDeleteResult& deleteResult,
        const ISearchTimerDataSource* dataSource) const;
};
