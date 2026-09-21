#include "SearchTimerWorkflowValidationResultJsonSerializer.h"
#include "SearchTimerWorkflowValidationService.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    SearchTimerWorkflowValidationService service;
    SearchTimerWorkflowValidationResultJsonSerializer serializer;

    const SearchTimerWorkflowValidationResult createResult =
        service.validate(
            SearchTimerWorkflowRequest::create(
                "livingroom",
                "Daily News",
                "Tagesschau"));

    const std::string createJson =
        serializer.serialize(createResult);

    assert(createJson.find("\"valid\":true") != std::string::npos);
    assert(createJson.find("\"operation\":\"create\"") != std::string::npos);
    assert(createJson.find("\"readOnly\":false") != std::string::npos);
    assert(createJson.find("\"writeOperation\":true") != std::string::npos);
    assert(createJson.find("\"wantsReadbackAfterWrite\":true") != std::string::npos);
    assert(createJson.find("\"backendId\":\"livingroom\"") != std::string::npos);
    assert(createJson.find("\"backendNativeId\":\"\"") != std::string::npos);
    assert(createJson.find("\"write operation requires explicit operator intent\"") != std::string::npos);
    assert(createJson.find("\"backend readback is recommended after this operation\"") != std::string::npos);
    assert(createJson.find("\"errors\":[]") != std::string::npos);

    const SearchTimerWorkflowValidationResult invalidUpdateResult =
        service.validate(
            SearchTimerWorkflowRequest::update(
                "livingroom",
                "",
                "Daily News Updated",
                "Tagesschau Heute"));

    const std::string invalidUpdateJson =
        serializer.serialize(invalidUpdateResult);

    assert(invalidUpdateJson.find("\"valid\":false") != std::string::npos);
    assert(invalidUpdateJson.find("\"operation\":\"update\"") != std::string::npos);
    assert(invalidUpdateJson.find("\"backendId\":\"livingroom\"") != std::string::npos);
    assert(invalidUpdateJson.find("\"backendNativeId\":\"\"") != std::string::npos);
    assert(invalidUpdateJson.find("\"backendNativeId is required\"") != std::string::npos);

    SearchTimerWorkflowValidationResult escapedResult =
        service.validate(
            SearchTimerWorkflowRequest::preview(
                "home\\vdr",
                "Quote \" Search",
                "Line\nBreak"));

    const std::string escapedJson =
        serializer.serialize(escapedResult);

    assert(escapedJson.find("\"operation\":\"preview\"") != std::string::npos);
    assert(escapedJson.find("\"backendId\":\"home\\\\vdr\"") != std::string::npos);

    std::cout
        << "test_search_timer_workflow_validation_result_json_serializer passed"
        << std::endl;

    return 0;
}
