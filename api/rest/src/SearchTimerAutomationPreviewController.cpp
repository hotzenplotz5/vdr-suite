#include "SearchTimerAutomationPreviewController.h"

#include "SearchTimerAutomationDryRunResultJsonSerializer.h"
#include "SearchTimerAutomationEvaluationPlan.h"
#include "SearchTimerAutomationReadOnlyService.h"

#include <vector>

SearchTimerAutomationPreviewController::SearchTimerAutomationPreviewController(
    SearchTimerAutomationDryRunResultJsonSerializer& jsonSerializer)
    : readOnlyService_(nullptr),
      jsonSerializer_(jsonSerializer)
{
}

SearchTimerAutomationPreviewController::SearchTimerAutomationPreviewController(
    SearchTimerAutomationReadOnlyService& readOnlyService,
    SearchTimerAutomationDryRunResultJsonSerializer& jsonSerializer)
    : readOnlyService_(&readOnlyService),
      jsonSerializer_(jsonSerializer)
{
}

ApiResponse SearchTimerAutomationPreviewController::preview(
    const SearchTimerAutomationDryRunResult& result) const
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = jsonSerializer_.serialize(result);

    return response;
}

ApiResponse SearchTimerAutomationPreviewController::getPreview(
    const std::string& backendId,
    int candidateLimit) const
{
    if (readOnlyService_ == nullptr)
    {
        ApiResponse response;
        response.statusCode = 503;
        response.contentType = "application/json";
        response.body =
            "{\"error\":\"SearchTimer automation preview service not configured\"}";
        return response;
    }

    SearchTimerAutomationEvaluationPlan plan =
        SearchTimerAutomationEvaluationPlan::createReadOnly(
            backendId.empty()
                ? "default"
                : backendId);

    plan.setCandidateLimit(candidateLimit);

    SearchTimerAutomationDryRunResult result =
        readOnlyService_->evaluate(
            plan,
            std::vector<SearchTimerAutomationMatchCandidate>{},
            std::vector<SearchTimerAutomationDuplicateDetection>{},
            std::vector<SearchTimerAutomationCandidateTimerProposal>{},
            0);

    result.addAuditEntry("REST automation preview contract rendered");

    return preview(result);
}
