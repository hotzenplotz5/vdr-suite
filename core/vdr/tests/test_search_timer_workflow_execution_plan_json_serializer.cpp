#include "SearchTimerWorkflowExecutionPlanJsonSerializer.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowExecutionPlanJsonSerializer serializer;

    const std::string listJson =
        serializer.serialize(
            planningService.plan(
                SearchTimerWorkflowRequest::list()));

    assert(listJson.find("\"valid\":true") != std::string::npos);
    assert(listJson.find("\"operation\":\"list\"") != std::string::npos);
    assert(listJson.find("\"executionMode\":\"prepare\"") != std::string::npos);
    assert(listJson.find("\"primaryStep\":\"list\"") != std::string::npos);
    assert(listJson.find("\"followUpStep\":\"none\"") != std::string::npos);
    assert(listJson.find("\"hasExecutionWork\":true") != std::string::npos);
    assert(listJson.find("\"hasFollowUpStep\":false") != std::string::npos);
    assert(listJson.find("\"readOnly\":true") != std::string::npos);
    assert(listJson.find("\"writeOperation\":false") != std::string::npos);
    assert(listJson.find("\"requiresExplicitOperatorConfirmation\":false") != std::string::npos);
    assert(listJson.find("\"requiresBackendReadback\":false") != std::string::npos);

    const std::string createJson =
        serializer.serialize(
            planningService.plan(
                SearchTimerWorkflowRequest::create(
                    "home-vdr",
                    "Terra X \"Spezial\"",
                    "Terra X\\HD",
                    false)));

    assert(createJson.find("\"valid\":true") != std::string::npos);
    assert(createJson.find("\"operation\":\"create\"") != std::string::npos);
    assert(createJson.find("\"executionMode\":\"prepare\"") != std::string::npos);
    assert(createJson.find("\"primaryStep\":\"create\"") != std::string::npos);
    assert(createJson.find("\"followUpStep\":\"readback\"") != std::string::npos);
    assert(createJson.find("\"hasFollowUpStep\":true") != std::string::npos);
    assert(createJson.find("\"readOnly\":false") != std::string::npos);
    assert(createJson.find("\"writeOperation\":true") != std::string::npos);
    assert(createJson.find("\"requiresExplicitOperatorConfirmation\":true") != std::string::npos);
    assert(createJson.find("\"requiresBackendReadback\":true") != std::string::npos);
    assert(createJson.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(createJson.find("\"name\":\"Terra X \\\"Spezial\\\"\"") != std::string::npos);
    assert(createJson.find("\"query\":\"Terra X\\\\HD\"") != std::string::npos);
    assert(createJson.find("\"active\":false") != std::string::npos);

    const std::string dryRunCreateJson =
        serializer.serialize(
            planningService.plan(
                SearchTimerWorkflowRequest::create(
                    "home-vdr",
                    "Terra X Dry",
                    "Terra X")
                    .withExecutionMode(SearchTimerWorkflowExecutionMode::DryRun)));

    assert(dryRunCreateJson.find("\"operation\":\"create\"") != std::string::npos);
    assert(dryRunCreateJson.find("\"executionMode\":\"dryRun\"") != std::string::npos);

    const std::string readbackJson =
        serializer.serialize(
            planningService.plan(
                SearchTimerWorkflowRequest::readback(
                    "home-vdr",
                    "searchtimer-42")));

    assert(readbackJson.find("\"operation\":\"readback\"") != std::string::npos);
    assert(readbackJson.find("\"primaryStep\":\"readback\"") != std::string::npos);
    assert(readbackJson.find("\"backendNativeId\":\"searchtimer-42\"") != std::string::npos);
    assert(readbackJson.find("\"readOnly\":true") != std::string::npos);
    assert(readbackJson.find("\"writeOperation\":false") != std::string::npos);

    const std::string updateJson =
        serializer.serialize(
            planningService.plan(
                SearchTimerWorkflowRequest::update(
                    "home-vdr",
                    "searchtimer-42",
                    "Terra X aktuell",
                    "Terra X aktuell")));

    assert(updateJson.find("\"operation\":\"update\"") != std::string::npos);
    assert(updateJson.find("\"primaryStep\":\"update\"") != std::string::npos);
    assert(updateJson.find("\"followUpStep\":\"readback\"") != std::string::npos);
    assert(updateJson.find("\"requiresBackendReadback\":true") != std::string::npos);

    const std::string deleteJson =
        serializer.serialize(
            planningService.plan(
                SearchTimerWorkflowRequest::remove(
                    "home-vdr",
                    "searchtimer-42")));

    assert(deleteJson.find("\"operation\":\"delete\"") != std::string::npos);
    assert(deleteJson.find("\"primaryStep\":\"delete\"") != std::string::npos);
    assert(deleteJson.find("\"followUpStep\":\"none\"") != std::string::npos);
    assert(deleteJson.find("\"requiresBackendReadback\":false") != std::string::npos);
    assert(deleteJson.find("\"requiresExplicitOperatorConfirmation\":true") != std::string::npos);

    const std::string invalidJson =
        serializer.serialize(
            planningService.plan(
                SearchTimerWorkflowRequest::update(
                    "home-vdr",
                    "",
                    "Terra X aktuell",
                    "Terra X aktuell")));

    assert(invalidJson.find("\"valid\":false") != std::string::npos);
    assert(invalidJson.find("\"operation\":\"update\"") != std::string::npos);
    assert(invalidJson.find("\"primaryStep\":\"none\"") != std::string::npos);
    assert(invalidJson.find("\"hasExecutionWork\":false") != std::string::npos);
    assert(invalidJson.find("\"writeOperation\":true") != std::string::npos);

    std::cout
        << "test_search_timer_workflow_execution_plan_json_serializer passed"
        << std::endl;

    return 0;
}
