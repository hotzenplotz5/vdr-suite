#include "SearchTimerWorkflowRealExecutionReadinessReviewJsonSerializer.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>
#include <string>

class ReadinessReviewJsonCommandExecutor : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest&) override
    {
        return SearchTimerCreateResult::failed(
            "unexpected create call",
            {"unexpected create call"});
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest&) override
    {
        return SearchTimerUpdateResult::failed(
            "unexpected update call",
            {"unexpected update call"});
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest&) override
    {
        return SearchTimerDeleteResult::failed(
            "unexpected delete call",
            {"unexpected delete call"});
    }
};

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowRealExecutionReadinessReview review;
    SearchTimerWorkflowRealExecutionReadinessReviewJsonSerializer serializer;
    ReadinessReviewJsonCommandExecutor executor;

    const SearchTimerWorkflowExecutionPlan executePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "json-vdr",
                "JSON Readiness",
                "JSON")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowRealExecutionReadinessReviewResult result =
        review.review(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocation(
                true,
                &executor));

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"readyForRealBackendExecution\":false") != std::string::npos);
    assert(json.find("\"planExecutable\":true") != std::string::npos);
    assert(json.find("\"writeOperation\":true") != std::string::npos);
    assert(json.find("\"executeModeRequested\":true") != std::string::npos);
    assert(json.find("\"executorOptInProvided\":true") != std::string::npos);
    assert(json.find("\"executorInjected\":true") != std::string::npos);
    assert(json.find("\"controlledTestInvocationOnly\":true") != std::string::npos);
    assert(json.find("\"productionRealExecutionPolicyAvailable\":false") != std::string::npos);
    assert(json.find("\"readinessStage\":\"real-backend-execution-not-ready\"") != std::string::npos);
    assert(json.find("controlled test invocation is not production real execution") != std::string::npos);
    assert(json.find("production real execution policy gate is not available") != std::string::npos);

    std::cout
        << "test_search_timer_workflow_real_execution_readiness_review_json_serializer passed"
        << std::endl;

    return 0;
}
