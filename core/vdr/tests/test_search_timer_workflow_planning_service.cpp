#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerWorkflowPlanningService service;

    const SearchTimerWorkflowExecutionPlan listPlan =
        service.plan(SearchTimerWorkflowRequest::list());

    assert(listPlan.valid());
    assert(listPlan.hasExecutionWork());
    assert(listPlan.operation() == SearchTimerWorkflowOperation::List);
    assert(listPlan.primaryStep() == SearchTimerWorkflowExecutionStep::List);
    assert(!listPlan.hasFollowUpStep());
    assert(listPlan.readOnly());
    assert(!listPlan.writeOperation());

    const SearchTimerWorkflowExecutionPlan previewPlan =
        service.plan(
            SearchTimerWorkflowRequest::preview(
                "home-vdr",
                "Terra X Suche",
                "Terra X"));

    assert(previewPlan.valid());
    assert(previewPlan.hasExecutionWork());
    assert(previewPlan.primaryStep() == SearchTimerWorkflowExecutionStep::Preview);
    assert(previewPlan.backendId() == "home-vdr");
    assert(previewPlan.name() == "Terra X Suche");
    assert(previewPlan.query() == "Terra X");
    assert(previewPlan.readOnly());
    assert(!previewPlan.writeOperation());

    const SearchTimerWorkflowExecutionPlan createPlan =
        service.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Suche",
                "Terra X",
                true));

    assert(createPlan.valid());
    assert(createPlan.hasExecutionWork());
    assert(createPlan.primaryStep() == SearchTimerWorkflowExecutionStep::Create);
    assert(createPlan.followUpStep() == SearchTimerWorkflowExecutionStep::Readback);
    assert(createPlan.hasFollowUpStep());
    assert(createPlan.requiresBackendReadback());
    assert(createPlan.requiresExplicitOperatorConfirmation());
    assert(createPlan.writeOperation());
    assert(!createPlan.readOnly());

    const SearchTimerWorkflowExecutionPlan readbackPlan =
        service.plan(
            SearchTimerWorkflowRequest::readback(
                "home-vdr",
                "searchtimer-42"));

    assert(readbackPlan.valid());
    assert(readbackPlan.hasExecutionWork());
    assert(readbackPlan.primaryStep() == SearchTimerWorkflowExecutionStep::Readback);
    assert(readbackPlan.backendNativeId() == "searchtimer-42");
    assert(readbackPlan.readOnly());
    assert(!readbackPlan.writeOperation());

    const SearchTimerWorkflowExecutionPlan updatePlan =
        service.plan(
            SearchTimerWorkflowRequest::update(
                "home-vdr",
                "searchtimer-42",
                "Terra X Suche aktuell",
                "Terra X aktuell",
                false));

    assert(updatePlan.valid());
    assert(updatePlan.hasExecutionWork());
    assert(updatePlan.primaryStep() == SearchTimerWorkflowExecutionStep::Update);
    assert(updatePlan.followUpStep() == SearchTimerWorkflowExecutionStep::Readback);
    assert(updatePlan.requiresBackendReadback());
    assert(updatePlan.requiresExplicitOperatorConfirmation());
    assert(updatePlan.writeOperation());
    assert(!updatePlan.readOnly());
    assert(!updatePlan.active());

    const SearchTimerWorkflowExecutionPlan deletePlan =
        service.plan(
            SearchTimerWorkflowRequest::remove(
                "home-vdr",
                "searchtimer-42"));

    assert(deletePlan.valid());
    assert(deletePlan.hasExecutionWork());
    assert(deletePlan.primaryStep() == SearchTimerWorkflowExecutionStep::Delete);
    assert(!deletePlan.hasFollowUpStep());
    assert(!deletePlan.requiresBackendReadback());
    assert(deletePlan.requiresExplicitOperatorConfirmation());
    assert(deletePlan.writeOperation());
    assert(!deletePlan.readOnly());

    const SearchTimerWorkflowExecutionPlan invalidUpdatePlan =
        service.plan(
            SearchTimerWorkflowRequest::update(
                "home-vdr",
                "",
                "Terra X Suche aktuell",
                "Terra X aktuell"));

    assert(!invalidUpdatePlan.valid());
    assert(!invalidUpdatePlan.hasExecutionWork());
    assert(invalidUpdatePlan.operation() == SearchTimerWorkflowOperation::Update);
    assert(invalidUpdatePlan.primaryStep() == SearchTimerWorkflowExecutionStep::None);
    assert(invalidUpdatePlan.followUpStep() == SearchTimerWorkflowExecutionStep::None);
    assert(invalidUpdatePlan.writeOperation());
    assert(!invalidUpdatePlan.readOnly());

    const SearchTimerWorkflowExecutionPlan unknownPlan =
        service.plan(SearchTimerWorkflowRequest());

    assert(!unknownPlan.valid());
    assert(!unknownPlan.hasExecutionWork());
    assert(unknownPlan.operation() == SearchTimerWorkflowOperation::Unknown);
    assert(unknownPlan.primaryStep() == SearchTimerWorkflowExecutionStep::None);

    std::cout
        << "test_search_timer_workflow_planning_service passed"
        << std::endl;

    return 0;
}
