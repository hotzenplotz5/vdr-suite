#include "SearchTimerWorkflowValidationRequestParser.h"

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
    assert(createRequest.isValid());

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

    std::cout
        << "test_search_timer_workflow_validation_request_parser passed"
        << std::endl;

    return 0;
}
