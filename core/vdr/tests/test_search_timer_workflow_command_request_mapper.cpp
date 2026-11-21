#include "SearchTimerWorkflowCommandRequestMapper.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowCommandRequestMapper mapper;

    const auto createPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Suche",
                "Terra X",
                false));

    assert(mapper.canBuildCreateRequest(createPlan));
    assert(!mapper.canBuildUpdateRequest(createPlan));
    assert(!mapper.canBuildDeleteRequest(createPlan));

    const SearchTimerCreateRequest createRequest =
        mapper.buildCreateRequest(createPlan);

    assert(createRequest.backendId == "home-vdr");
    assert(createRequest.name == "Terra X Suche");
    assert(createRequest.query == "Terra X");
    assert(!createRequest.active);
    assert(!createRequest.compareTitle);
    assert(!createRequest.compareSubtitle);
    assert(!createRequest.compareSummary);
    assert(!createRequest.compareCategories);

    const auto updatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::update(
                "remote-vdr",
                "searchtimer-42",
                "Terra X Aktualisiert",
                "Terra X neu",
                true));

    assert(!mapper.canBuildCreateRequest(updatePlan));
    assert(mapper.canBuildUpdateRequest(updatePlan));
    assert(!mapper.canBuildDeleteRequest(updatePlan));

    const SearchTimerUpdateRequest updateRequest =
        mapper.buildUpdateRequest(updatePlan);

    assert(updateRequest.backendId == "remote-vdr");
    assert(updateRequest.backendNativeId == "searchtimer-42");
    assert(updateRequest.name == "Terra X Aktualisiert");
    assert(updateRequest.query == "Terra X neu");
    assert(updateRequest.active);
    assert(!updateRequest.compareTitle);
    assert(!updateRequest.compareSubtitle);
    assert(!updateRequest.compareSummary);
    assert(!updateRequest.compareCategories);


    const auto titleOnlyWorkflowCreatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Amerika",
                "Amerika",
                true)
                .withCompareFields(
                    true,
                    false,
                    false,
                    false));

    assert(mapper.canBuildCreateRequest(titleOnlyWorkflowCreatePlan));

    const SearchTimerCreateRequest titleOnlyWorkflowCreateRequest =
        mapper.buildCreateRequest(titleOnlyWorkflowCreatePlan);

    assert(titleOnlyWorkflowCreateRequest.backendId == "home-vdr");
    assert(titleOnlyWorkflowCreateRequest.name == "Amerika");
    assert(titleOnlyWorkflowCreateRequest.query == "Amerika");
    assert(titleOnlyWorkflowCreateRequest.active);
    assert(titleOnlyWorkflowCreateRequest.compareTitle);
    assert(!titleOnlyWorkflowCreateRequest.compareSubtitle);
    assert(!titleOnlyWorkflowCreateRequest.compareSummary);
    assert(!titleOnlyWorkflowCreateRequest.compareCategories);

    const auto titleOnlyWorkflowUpdatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::update(
                "home-vdr",
                "searchtimer-amerika",
                "Amerika",
                "Amerika",
                true)
                .withCompareFields(
                    true,
                    false,
                    false,
                    false));

    assert(mapper.canBuildUpdateRequest(titleOnlyWorkflowUpdatePlan));

    const SearchTimerUpdateRequest titleOnlyWorkflowUpdateRequest =
        mapper.buildUpdateRequest(titleOnlyWorkflowUpdatePlan);

    assert(titleOnlyWorkflowUpdateRequest.backendId == "home-vdr");
    assert(titleOnlyWorkflowUpdateRequest.backendNativeId == "searchtimer-amerika");
    assert(titleOnlyWorkflowUpdateRequest.name == "Amerika");
    assert(titleOnlyWorkflowUpdateRequest.query == "Amerika");
    assert(titleOnlyWorkflowUpdateRequest.active);
    assert(titleOnlyWorkflowUpdateRequest.compareTitle);
    assert(!titleOnlyWorkflowUpdateRequest.compareSubtitle);
    assert(!titleOnlyWorkflowUpdateRequest.compareSummary);
    assert(!titleOnlyWorkflowUpdateRequest.compareCategories);

    const auto deletePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::remove(
                "archive-vdr",
                "searchtimer-99"));

    assert(!mapper.canBuildCreateRequest(deletePlan));
    assert(!mapper.canBuildUpdateRequest(deletePlan));
    assert(mapper.canBuildDeleteRequest(deletePlan));

    const SearchTimerDeleteRequest deleteRequest =
        mapper.buildDeleteRequest(deletePlan);

    assert(deleteRequest.backendId == "archive-vdr");
    assert(deleteRequest.backendNativeId == "searchtimer-99");

    const auto listPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::list());

    assert(!mapper.canBuildCreateRequest(listPlan));
    assert(!mapper.canBuildUpdateRequest(listPlan));
    assert(!mapper.canBuildDeleteRequest(listPlan));

    const auto invalidUpdatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::update(
                "home-vdr",
                "",
                "Terra X",
                "Terra X"));

    assert(!mapper.canBuildCreateRequest(invalidUpdatePlan));
    assert(!mapper.canBuildUpdateRequest(invalidUpdatePlan));
    assert(!mapper.canBuildDeleteRequest(invalidUpdatePlan));

    std::cout
        << "test_search_timer_workflow_command_request_mapper passed"
        << std::endl;

    return 0;
}
