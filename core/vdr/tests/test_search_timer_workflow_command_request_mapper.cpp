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
