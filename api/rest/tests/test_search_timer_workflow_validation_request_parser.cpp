#include "SearchTimerWorkflowValidationRequestParser.h"
#include "SearchTimerWorkflowExecutionPlan.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerWorkflowValidationRequestParser parser;

    SearchTimerWorkflowRequest listRequest =
        parser.parse("{\"operation\":\"list\"}");

    assert(listRequest.operation() == SearchTimerWorkflowOperation::List);
    assert(listRequest.isReadOnly());
    assert(!listRequest.isWriteOperation());
    assert(listRequest.isValid());

    SearchTimerWorkflowRequest previewRequest =
        parser.parse(
            "{"
            "\"operation\":\"preview\","
            "\"backendId\":\"home-vdr\","
            "\"name\":\"Terra X Suche\","
            "\"query\":\"Terra X\""
            "}");

    assert(previewRequest.operation() == SearchTimerWorkflowOperation::Preview);
    assert(previewRequest.backendId() == "home-vdr");
    assert(previewRequest.name() == "Terra X Suche");
    assert(previewRequest.query() == "Terra X");
    assert(previewRequest.isReadOnly());
    assert(previewRequest.isValid());

    SearchTimerWorkflowRequest createRequest =
        parser.parse(
            "{"
            "\"action\":\"create\","
            "\"backendId\":\"home-vdr\","
            "\"name\":\"Terra X Suche\","
            "\"query\":\"Terra X\","
            "\"active\":false"
            "}");

    assert(createRequest.operation() == SearchTimerWorkflowOperation::Create);
    assert(createRequest.backendId() == "home-vdr");
    assert(createRequest.name() == "Terra X Suche");
    assert(createRequest.query() == "Terra X");
    assert(!createRequest.active());
    assert(createRequest.isWriteOperation());
    assert(createRequest.wantsReadbackAfterWrite());
    assert(createRequest.executionMode() == SearchTimerWorkflowExecutionMode::Prepare);
    assert(createRequest.isValid());

    SearchTimerWorkflowRequest dryRunCreateRequest =
        parser.parse(
            "{"
            "\"operation\":\"create\","
            "\"backendId\":\"home-vdr\","
            "\"name\":\"Terra X Dry\","
            "\"query\":\"Terra X\","
            "\"executionMode\":\"dryRun\""
            "}");

    assert(dryRunCreateRequest.operation() == SearchTimerWorkflowOperation::Create);
    assert(dryRunCreateRequest.executionMode() == SearchTimerWorkflowExecutionMode::DryRun);
    assert(dryRunCreateRequest.dryRunMode());
    assert(dryRunCreateRequest.isValid());

    SearchTimerWorkflowRequest executeDeleteRequest =
        parser.parse(
            "{"
            "\"operation\":\"delete\","
            "\"backendId\":\"home-vdr\","
            "\"backendNativeId\":\"searchtimer-42\","
            "\"mode\":\"execute\""
            "}");

    assert(executeDeleteRequest.operation() == SearchTimerWorkflowOperation::Delete);
    assert(executeDeleteRequest.executionMode() == SearchTimerWorkflowExecutionMode::Execute);
    assert(executeDeleteRequest.executeMode());
    assert(executeDeleteRequest.isValid());

    SearchTimerWorkflowRequest readbackRequest =
        parser.parse(
            "{"
            "\"operation\":\"readback\","
            "\"backendId\":\"home-vdr\","
            "\"backendNativeId\":\"searchtimer-42\""
            "}");

    assert(readbackRequest.operation() == SearchTimerWorkflowOperation::Readback);
    assert(readbackRequest.backendNativeId() == "searchtimer-42");
    assert(readbackRequest.isReadOnly());
    assert(readbackRequest.isValid());

    SearchTimerWorkflowRequest updateRequest =
        parser.parse(
            "{"
            "\"operation\":\"update\","
            "\"backendId\":\"home-vdr\","
            "\"backendNativeId\":\"searchtimer-42\","
            "\"name\":\"Terra X Suche aktuell\","
            "\"query\":\"Terra X aktuell\","
            "\"active\":true"
            "}");

    assert(updateRequest.operation() == SearchTimerWorkflowOperation::Update);
    assert(updateRequest.backendNativeId() == "searchtimer-42");
    assert(updateRequest.active());
    assert(updateRequest.isWriteOperation());
    assert(updateRequest.wantsReadbackAfterWrite());
    assert(updateRequest.isValid());

    SearchTimerWorkflowRequest removeRequest =
        parser.parse(
            "{"
            "\"operation\":\"remove\","
            "\"backendId\":\"home-vdr\","
            "\"backendNativeId\":\"searchtimer-42\""
            "}");

    assert(removeRequest.operation() == SearchTimerWorkflowOperation::Delete);
    assert(removeRequest.backendNativeId() == "searchtimer-42");
    assert(removeRequest.isWriteOperation());
    assert(removeRequest.isValid());

    SearchTimerWorkflowRequest invalidUpdateRequest =
        parser.parse(
            "{"
            "\"operation\":\"update\","
            "\"backendId\":\"home-vdr\","
            "\"name\":\"Terra X Suche aktuell\","
            "\"query\":\"Terra X aktuell\""
            "}");

    assert(invalidUpdateRequest.operation() == SearchTimerWorkflowOperation::Update);
    assert(!invalidUpdateRequest.hasBackendNativeId());
    assert(!invalidUpdateRequest.isValid());

    SearchTimerWorkflowRequest unknownRequest =
        parser.parse("{\"operation\":\"sideways\"}");

    assert(unknownRequest.operation() == SearchTimerWorkflowOperation::Unknown);
    assert(!unknownRequest.hasOperation());
    assert(!unknownRequest.isValid());


    SearchTimerWorkflowRequest titleOnlyWorkflowCreateRequest =
        parser.parse(
            "{"
            "\"operation\":\"create\","
            "\"backendId\":\"home-vdr\","
            "\"name\":\"Amerika\","
            "\"query\":\"Amerika\","
            "\"active\":true,"
            "\"compareTitle\":true,"
            "\"compareSubtitle\":false,"
            "\"compareSummary\":false,"
            "\"compareCategories\":false"
            "}");

    assert(titleOnlyWorkflowCreateRequest.operation() == SearchTimerWorkflowOperation::Create);
    assert(titleOnlyWorkflowCreateRequest.backendId() == "home-vdr");
    assert(titleOnlyWorkflowCreateRequest.name() == "Amerika");
    assert(titleOnlyWorkflowCreateRequest.query() == "Amerika");
    assert(titleOnlyWorkflowCreateRequest.compareTitle());
    assert(!titleOnlyWorkflowCreateRequest.compareSubtitle());
    assert(!titleOnlyWorkflowCreateRequest.compareSummary());
    assert(!titleOnlyWorkflowCreateRequest.compareCategories());
    assert(titleOnlyWorkflowCreateRequest.isValid());

    SearchTimerWorkflowExecutionPlan titleOnlyWorkflowCreatePlan =
        SearchTimerWorkflowExecutionPlan::fromRequest(
            titleOnlyWorkflowCreateRequest);

    assert(titleOnlyWorkflowCreatePlan.valid());
    assert(titleOnlyWorkflowCreatePlan.primaryStep() == SearchTimerWorkflowExecutionStep::Create);
    assert(titleOnlyWorkflowCreatePlan.compareTitle());
    assert(!titleOnlyWorkflowCreatePlan.compareSubtitle());
    assert(!titleOnlyWorkflowCreatePlan.compareSummary());
    assert(!titleOnlyWorkflowCreatePlan.compareCategories());

    SearchTimerWorkflowRequest titleOnlyWorkflowUpdateRequest =
        parser.parse(
            "{"
            "\"operation\":\"update\","
            "\"backendId\":\"home-vdr\","
            "\"backendNativeId\":\"searchtimer-amerika\","
            "\"name\":\"Amerika\","
            "\"query\":\"Amerika\","
            "\"active\":true,"
            "\"compareTitle\":true,"
            "\"compareSubtitle\":false,"
            "\"compareSummary\":false,"
            "\"compareCategories\":false"
            "}");

    assert(titleOnlyWorkflowUpdateRequest.operation() == SearchTimerWorkflowOperation::Update);
    assert(titleOnlyWorkflowUpdateRequest.backendNativeId() == "searchtimer-amerika");
    assert(titleOnlyWorkflowUpdateRequest.compareTitle());
    assert(!titleOnlyWorkflowUpdateRequest.compareSubtitle());
    assert(!titleOnlyWorkflowUpdateRequest.compareSummary());
    assert(!titleOnlyWorkflowUpdateRequest.compareCategories());
    assert(titleOnlyWorkflowUpdateRequest.isValid());

    SearchTimerWorkflowExecutionPlan titleOnlyWorkflowUpdatePlan =
        SearchTimerWorkflowExecutionPlan::fromRequest(
            titleOnlyWorkflowUpdateRequest);

    assert(titleOnlyWorkflowUpdatePlan.valid());
    assert(titleOnlyWorkflowUpdatePlan.primaryStep() == SearchTimerWorkflowExecutionStep::Update);
    assert(titleOnlyWorkflowUpdatePlan.compareTitle());
    assert(!titleOnlyWorkflowUpdatePlan.compareSubtitle());
    assert(!titleOnlyWorkflowUpdatePlan.compareSummary());
    assert(!titleOnlyWorkflowUpdatePlan.compareCategories());

    std::cout
        << "test_search_timer_workflow_validation_request_parser passed"
        << std::endl;

    return 0;
}
