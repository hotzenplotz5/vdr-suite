#pragma once

#include "DashboardController.h"
#include "SearchTimerAutomationDryRunResult.h"

#include <string>

class SearchTimerAutomationDryRunResultJsonSerializer;
class SearchTimerAutomationReadOnlyService;

class SearchTimerAutomationPreviewController
{
public:
    explicit SearchTimerAutomationPreviewController(
        SearchTimerAutomationDryRunResultJsonSerializer& jsonSerializer);

    SearchTimerAutomationPreviewController(
        SearchTimerAutomationReadOnlyService& readOnlyService,
        SearchTimerAutomationDryRunResultJsonSerializer& jsonSerializer);

    ApiResponse preview(
        const SearchTimerAutomationDryRunResult& result) const;

    ApiResponse getPreview(
        const std::string& backendId,
        int candidateLimit) const;

private:
    SearchTimerAutomationReadOnlyService* readOnlyService_;
    SearchTimerAutomationDryRunResultJsonSerializer& jsonSerializer_;
};
