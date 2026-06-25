#include "SearchTimerAutomationDryRunResultJsonSerializer.h"
#include "SearchTimerAutomationPreviewController.h"
#include "SearchTimerAutomationReadOnlyService.h"

#include <cassert>
#include <iostream>
#include <string>

namespace
{

bool contains(
    const std::string& haystack,
    const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

} // namespace

int main()
{
    SearchTimerAutomationDryRunResultJsonSerializer serializer;

    SearchTimerAutomationPreviewController unavailableController(
        serializer);

    ApiResponse unavailableResponse =
        unavailableController.getPreview(
            "default",
            25);

    assert(unavailableResponse.statusCode == 503);
    assert(unavailableResponse.contentType == "application/json");
    assert(contains(
        unavailableResponse.body,
        "not configured"));

    SearchTimerAutomationReadOnlyService service;

    SearchTimerAutomationPreviewController controller(
        service,
        serializer);

    ApiResponse response =
        controller.getPreview(
            "livingroom",
            25);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(contains(response.body, "\"success\":true"));
    assert(contains(response.body, "\"dryRunOnly\":true"));
    assert(contains(response.body, "\"mutationAllowed\":false"));
    assert(contains(response.body, "\"timerCreationAllowed\":false"));
    assert(contains(response.body, "\"backendWriteAllowed\":false"));
    assert(contains(response.body, "\"automaticExecutionAllowed\":false"));
    assert(contains(response.body, "\"requiresExplicitExecutionHandoff\":true"));
    assert(contains(response.body, "\"backendId\":\"livingroom\""));
    assert(contains(response.body, "\"candidateLimit\":25"));
    assert(contains(response.body, "\"matchedCandidateCount\":0"));
    assert(contains(response.body, "\"duplicateRiskCount\":0"));
    assert(contains(response.body, "\"proposalCount\":0"));
    assert(contains(response.body, "\"matchCandidates\":[]"));
    assert(contains(response.body, "\"duplicateDetections\":[]"));
    assert(contains(response.body, "\"candidateTimerProposals\":[]"));
    assert(contains(response.body, "REST automation preview contract rendered"));

    ApiResponse defaultBackendResponse =
        controller.getPreview(
            "",
            0);

    assert(defaultBackendResponse.statusCode == 200);
    assert(contains(defaultBackendResponse.body, "\"backendId\":\"default\""));
    assert(contains(defaultBackendResponse.body, "\"candidateLimit\":1"));

    std::cout
        << "test_search_timer_automation_preview_controller passed"
        << std::endl;

    return 0;
}
