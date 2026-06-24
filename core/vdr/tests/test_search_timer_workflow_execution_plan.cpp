#include "SearchTimerWorkflowExecutionPlan.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerWorkflowExecutionPlan emptyPlan =
        SearchTimerWorkflowExecutionPlan::none();

    assert(!emptyPlan.valid());
    assert(!emptyPlan.hasExecutionWork());
    assert(emptyPlan.primaryStep() == SearchTimerWorkflowExecutionStep::None);
    assert(emptyPlan.followUpStep() == SearchTimerWorkflowExecutionStep::None);
    assert(emptyPlan.readOnly());
    assert(!emptyPlan.writeOperation());

    SearchTimerWorkflowExecutionPlan listPlan =
        SearchTimerWorkflowExecutionPlan::fromRequest(
            SearchTimerWorkflowRequest::list());

    assert(listPlan.valid());
    assert(listPlan.hasExecutionWork());
    assert(listPlan.operation() == SearchTimerWorkflowOperation::List);
    assert(listPlan.primaryStep() == SearchTimerWorkflowExecutionStep::List);
    assert(!listPlan.hasFollowUpStep());
    assert(listPlan.readOnly());
    assert(!listPlan.writeOperation());
    assert(!listPlan.requiresExplicitOperatorConfirmation());
    assert(!listPlan.requiresBackendReadback());

    SearchTimerWorkflowExecutionPlan previewPlan =
        SearchTimerWorkflowExecutionPlan::fromRequest(
            SearchTimerWorkflowRequest::preview(
                "home-vdr",
                "Terra X Suche",
                "Terra X"));

    assert(previewPlan.valid());
    assert(previewPlan.primaryStep() == SearchTimerWorkflowExecutionStep::Preview);
    assert(!previewPlan.hasFollowUpStep());
    assert(previewPlan.backendId() == "home-vdr");
    assert(previewPlan.name() == "Terra X Suche");
    assert(previewPlan.query() == "Terra X");
    assert(previewPlan.readOnly());
    assert(!previewPlan.writeOperation());

    SearchTimerWorkflowExecutionPlan createPlan =
        SearchTimerWorkflowExecutionPlan::fromRequest(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Suche",
                "Terra X",
                false));

    assert(createPlan.valid());
    assert(createPlan.primaryStep() == SearchTimerWorkflowExecutionStep::Create);
    assert(createPlan.followUpStep() == SearchTimerWorkflowExecutionStep::Readback);
    assert(createPlan.hasFollowUpStep());
    assert(createPlan.requiresBackendReadback());
    assert(createPlan.requiresExplicitOperatorConfirmation());
    assert(createPlan.writeOperation());
    assert(!createPlan.readOnly());
    assert(!createPlan.active());

    SearchTimerWorkflowExecutionPlan readbackPlan =
        SearchTimerWorkflowExecutionPlan::fromRequest(
            SearchTimerWorkflowRequest::readback(
                "home-vdr",
                "searchtimer-42"));

    assert(readbackPlan.valid());
    assert(readbackPlan.primaryStep() == SearchTimerWorkflowExecutionStep::Readback);
    assert(!readbackPlan.hasFollowUpStep());
    assert(readbackPlan.backendNativeId() == "searchtimer-42");
    assert(readbackPlan.readOnly());
    assert(!readbackPlan.writeOperation());

    SearchTimerWorkflowExecutionPlan updatePlan =
        SearchTimerWorkflowExecutionPlan::fromRequest(
            SearchTimerWorkflowRequest::update(
                "home-vdr",
                "searchtimer-42",
                "Terra X Suche aktuell",
                "Terra X aktuell"));

    assert(updatePlan.valid());
    assert(updatePlan.primaryStep() == SearchTimerWorkflowExecutionStep::Update);
    assert(updatePlan.followUpStep() == SearchTimerWorkflowExecutionStep::Readback);
    assert(updatePlan.hasFollowUpStep());
    assert(updatePlan.requiresBackendReadback());
    assert(updatePlan.requiresExplicitOperatorConfirmation());
    assert(updatePlan.writeOperation());
    assert(!updatePlan.readOnly());

    SearchTimerWorkflowExecutionPlan deletePlan =
        SearchTimerWorkflowExecutionPlan::fromRequest(
            SearchTimerWorkflowRequest::remove(
                "home-vdr",
                "searchtimer-42"));

    assert(deletePlan.valid());
    assert(deletePlan.primaryStep() == SearchTimerWorkflowExecutionStep::Delete);
    assert(!deletePlan.hasFollowUpStep());
    assert(!deletePlan.requiresBackendReadback());
    assert(deletePlan.requiresExplicitOperatorConfirmation());
    assert(deletePlan.writeOperation());
    assert(!deletePlan.readOnly());

    SearchTimerWorkflowExecutionPlan invalidPlan =
        SearchTimerWorkflowExecutionPlan::fromRequest(
            SearchTimerWorkflowRequest::update(
                "home-vdr",
                "",
                "Terra X Suche aktuell",
                "Terra X aktuell"));

    assert(!invalidPlan.valid());
    assert(!invalidPlan.hasExecutionWork());
    assert(invalidPlan.operation() == SearchTimerWorkflowOperation::Update);
    assert(invalidPlan.primaryStep() == SearchTimerWorkflowExecutionStep::None);
    assert(invalidPlan.followUpStep() == SearchTimerWorkflowExecutionStep::None);
    assert(invalidPlan.writeOperation());
    assert(!invalidPlan.readOnly());

    std::cout
        << "test_search_timer_workflow_execution_plan passed"
        << std::endl;

    return 0;
}
