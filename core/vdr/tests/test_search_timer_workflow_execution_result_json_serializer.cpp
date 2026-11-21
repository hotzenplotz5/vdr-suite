#include "SearchTimerWorkflowExecutionResultJsonSerializer.h"

#include "SearchTimerWorkflowExecutionService.h"
#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowExecutionService executionService;
    SearchTimerWorkflowExecutionResultJsonSerializer serializer;

    const SearchTimerWorkflowExecutionPlan createPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Suche",
                "Terra X"));

    const SearchTimerWorkflowExecutionResult blockedResult =
        executionService.executePlan(createPlan);

    const std::string blockedJson =
        serializer.serialize(blockedResult);

    assert(blockedJson.find("\"success\":false") != std::string::npos);
    assert(blockedJson.find("\"executed\":false") != std::string::npos);
    assert(blockedJson.find("\"blocked\":true") != std::string::npos);
    assert(blockedJson.find("\"dryRunOnly\":true") != std::string::npos);
    assert(blockedJson.find("\"confirmationProvided\":false") != std::string::npos);
    assert(blockedJson.find("\"requiresExplicitOperatorConfirmation\":true") != std::string::npos);
    assert(blockedJson.find("\"requiresBackendReadback\":true") != std::string::npos);
    assert(blockedJson.find("\"backendReadbackVerificationAttached\":false") != std::string::npos);
    assert(blockedJson.find("\"backendReadbackVerified\":false") != std::string::npos);
    assert(blockedJson.find("\"backendReadbackVerification\":{\"required\":false") != std::string::npos);
    assert(blockedJson.find("\"commandRequestMapped\":false") != std::string::npos);
    assert(blockedJson.find("\"realExecutionEnabled\":false") != std::string::npos);
    assert(blockedJson.find("\"realExecutionPolicyAllowed\":false") != std::string::npos);
    assert(blockedJson.find("\"executorOptInProvided\":false") != std::string::npos);
    assert(blockedJson.find("\"executorInjected\":false") != std::string::npos);
    assert(blockedJson.find("\"executorInvocationGuardPassed\":false") != std::string::npos);
    assert(blockedJson.find("\"executorInvocationAttempted\":false") != std::string::npos);
    assert(blockedJson.find("\"executorInvocationKillSwitchOpen\":false") != std::string::npos);
    assert(blockedJson.find("\"executorInvocationKillSwitchPassed\":false") != std::string::npos);
    assert(blockedJson.find("\"executorResultMapped\":false") != std::string::npos);
    assert(blockedJson.find("\"executorResultSuccessful\":false") != std::string::npos);
    assert(blockedJson.find("\"executorInvocationAuditTrail\":[]") != std::string::npos);
    assert(blockedJson.find("\"dispatchStage\":\"blocked\"") != std::string::npos);
    assert(blockedJson.find("\"executionMode\":\"prepare\"") != std::string::npos);
    assert(blockedJson.find("\"operation\":\"create\"") != std::string::npos);
    assert(blockedJson.find("\"primaryStep\":\"create\"") != std::string::npos);
    assert(blockedJson.find("\"followUpStep\":\"readback\"") != std::string::npos);
    assert(blockedJson.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(blockedJson.find("explicit operator confirmation is required") != std::string::npos);
    assert(blockedJson.find("\"warnings\":[]") != std::string::npos);
    assert(blockedJson.find("\"errors\":[\"explicit operator confirmation is required\"]") != std::string::npos);

    SearchTimerWorkflowExecutionResult acceptedResult =
        executionService.executePlan(createPlan, true);

    acceptedResult.message = "accepted quote message";
    acceptedResult.warnings.push_back("warning quoted message");

    const std::string acceptedJson =
        serializer.serialize(acceptedResult);

    assert(acceptedJson.find("\"success\":true") != std::string::npos);
    assert(acceptedJson.find("\"executed\":false") != std::string::npos);
    assert(acceptedJson.find("\"blocked\":false") != std::string::npos);
    assert(acceptedJson.find("\"confirmationProvided\":true") != std::string::npos);
    assert(acceptedJson.find("\"backendReadbackVerificationAttached\":false") != std::string::npos);
    assert(acceptedJson.find("\"backendReadbackVerified\":false") != std::string::npos);
    assert(acceptedJson.find("\"commandRequestMapped\":false") != std::string::npos);
    assert(acceptedJson.find("\"realExecutionEnabled\":false") != std::string::npos);
    assert(acceptedJson.find("\"realExecutionPolicyAllowed\":false") != std::string::npos);
    assert(acceptedJson.find("\"executorOptInProvided\":false") != std::string::npos);
    assert(acceptedJson.find("\"executorInjected\":false") != std::string::npos);
    assert(acceptedJson.find("\"executorInvocationGuardPassed\":false") != std::string::npos);
    assert(acceptedJson.find("\"executorInvocationAttempted\":false") != std::string::npos);
    assert(acceptedJson.find("\"executorInvocationKillSwitchOpen\":false") != std::string::npos);
    assert(acceptedJson.find("\"executorInvocationKillSwitchPassed\":false") != std::string::npos);
    assert(acceptedJson.find("\"executorResultMapped\":false") != std::string::npos);
    assert(acceptedJson.find("\"executorResultSuccessful\":false") != std::string::npos);
    assert(acceptedJson.find("\"executorInvocationAuditTrail\":[]") != std::string::npos);
    assert(acceptedJson.find("\"dispatchStage\":\"skeleton-accepted\"") != std::string::npos);
    assert(acceptedJson.find("\"executionMode\":\"prepare\"") != std::string::npos);
    assert(acceptedJson.find("\"message\":\"accepted quote message\"") != std::string::npos);
    assert(acceptedJson.find("backend execution is not implemented in this skeleton") != std::string::npos);
    assert(acceptedJson.find("backend readback will be required after real execution") != std::string::npos);
    assert(acceptedJson.find("warning quoted message") != std::string::npos);
    assert(acceptedJson.find("\"errors\":[]") != std::string::npos);

    SearchTimerWorkflowExecutionResult escapedResult = acceptedResult;
    escapedResult.message = "accepted \"quote\" message";

    const std::string escapedJson =
        serializer.serialize(escapedResult);

    assert(escapedJson.find("accepted \\\"quote\\\" message") != std::string::npos);


    SearchTimerWorkflowExecutionResult verifiedReadbackResult =
        acceptedResult;
    verifiedReadbackResult.attachBackendReadbackVerification(
        SearchTimerWorkflowBackendReadbackVerificationResult::verified(
            "home-vdr",
            "native-42",
            "native-42",
            "backend readback verified"));

    assert(verifiedReadbackResult.success);
    assert(verifiedReadbackResult.backendReadbackVerified());
    assert(verifiedReadbackResult.backendReadbackVerificationAttached);

    const std::string verifiedReadbackJson =
        serializer.serialize(verifiedReadbackResult);

    assert(verifiedReadbackJson.find("\"backendReadbackVerificationAttached\":true") != std::string::npos);
    assert(verifiedReadbackJson.find("\"backendReadbackVerified\":true") != std::string::npos);
    assert(verifiedReadbackJson.find("\"expectedBackendId\":\"home-vdr\"") != std::string::npos);
    assert(verifiedReadbackJson.find("\"expectedBackendNativeId\":\"native-42\"") != std::string::npos);
    assert(verifiedReadbackJson.find("\"observedBackendNativeId\":\"native-42\"") != std::string::npos);
    assert(verifiedReadbackJson.find("backend readback verified") != std::string::npos);

    SearchTimerWorkflowExecutionResult failedReadbackResult =
        acceptedResult;
    failedReadbackResult.attachBackendReadbackVerification(
        SearchTimerWorkflowBackendReadbackVerificationResult::failed(
            "home-vdr",
            "native-42",
            "",
            "backend readback mismatch",
            {"backend readback mismatch"}));

    assert(!failedReadbackResult.success);
    assert(!failedReadbackResult.backendReadbackVerified());
    assert(failedReadbackResult.backendReadbackVerificationAttached);
    assert(failedReadbackResult.hasErrors());

    const std::string failedReadbackJson =
        serializer.serialize(failedReadbackResult);

    assert(failedReadbackJson.find("\"success\":false") != std::string::npos);
    assert(failedReadbackJson.find("\"backendReadbackVerificationAttached\":true") != std::string::npos);
    assert(failedReadbackJson.find("\"backendReadbackVerified\":false") != std::string::npos);
    assert(failedReadbackJson.find("\"matched\":false") != std::string::npos);
    assert(failedReadbackJson.find("backend readback mismatch") != std::string::npos);

    const SearchTimerWorkflowExecutionResult invalidResult =
        executionService.executePlan(
            planningService.plan(
                SearchTimerWorkflowRequest::update(
                    "home-vdr",
                    "",
                    "Terra X Suche",
                    "Terra X")));

    const std::string invalidJson =
        serializer.serialize(invalidResult);

    assert(invalidJson.find("\"success\":false") != std::string::npos);
    assert(invalidJson.find("\"operation\":\"update\"") != std::string::npos);
    assert(invalidJson.find("\"primaryStep\":\"none\"") != std::string::npos);
    assert(invalidJson.find("\"dispatchStage\":\"blocked\"") != std::string::npos);
    assert(invalidJson.find("workflow plan is not executable") != std::string::npos);

    std::cout
        << "test_search_timer_workflow_execution_result_json_serializer passed"
        << std::endl;

    return 0;
}
