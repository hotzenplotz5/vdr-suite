#include "SearchTimerWorkflowValidationService.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{

bool contains(
    const std::vector<std::string>& values,
    const std::string& value)
{
    return std::find(
        values.begin(),
        values.end(),
        value) != values.end();
}

}

int main()
{
    SearchTimerWorkflowValidationService service;

    SearchTimerWorkflowValidationResult listAll =
        service.validate(SearchTimerWorkflowRequest::list());

    assert(listAll.valid);
    assert(listAll.operation == SearchTimerWorkflowOperation::List);
    assert(listAll.readOnly);
    assert(!listAll.writeOperation);
    assert(!listAll.wantsReadbackAfterWrite);
    assert(listAll.backendId.empty());
    assert(listAll.backendNativeId.empty());
    assert(listAll.errors.empty());
    assert(listAll.warnings.empty());

    SearchTimerWorkflowValidationResult preview =
        service.validate(
            SearchTimerWorkflowRequest::preview(
                "livingroom",
                "Preview SearchTimer",
                "Tatort"));

    assert(preview.valid);
    assert(preview.readOnly);
    assert(!preview.writeOperation);
    assert(preview.backendId == "livingroom");
    assert(preview.errors.empty());

    SearchTimerWorkflowValidationResult invalidPreview =
        service.validate(
            SearchTimerWorkflowRequest::preview(
                "",
                "Preview SearchTimer",
                "Tatort"));

    assert(!invalidPreview.valid);
    assert(contains(invalidPreview.errors, "backendId is required"));

    SearchTimerWorkflowValidationResult invalidCreate =
        service.validate(
            SearchTimerWorkflowRequest::create(
                "livingroom",
                "",
                ""));

    assert(!invalidCreate.valid);
    assert(invalidCreate.writeOperation);
    assert(contains(invalidCreate.errors, "name is required"));
    assert(contains(invalidCreate.errors, "query is required"));
    assert(contains(
        invalidCreate.warnings,
        "write operation requires explicit operator intent"));
    assert(contains(
        invalidCreate.warnings,
        "backend readback is recommended after this operation"));

    SearchTimerWorkflowValidationResult create =
        service.validate(
            SearchTimerWorkflowRequest::create(
                "livingroom",
                "Daily News",
                "Tagesschau"));

    assert(create.valid);
    assert(!create.readOnly);
    assert(create.writeOperation);
    assert(create.wantsReadbackAfterWrite);
    assert(create.backendId == "livingroom");
    assert(create.backendNativeId.empty());
    assert(create.errors.empty());
    assert(contains(
        create.warnings,
        "write operation requires explicit operator intent"));
    assert(contains(
        create.warnings,
        "backend readback is recommended after this operation"));

    SearchTimerWorkflowValidationResult readback =
        service.validate(
            SearchTimerWorkflowRequest::readback(
                "livingroom",
                "42"));

    assert(readback.valid);
    assert(readback.readOnly);
    assert(!readback.writeOperation);
    assert(!readback.wantsReadbackAfterWrite);
    assert(readback.backendId == "livingroom");
    assert(readback.backendNativeId == "42");
    assert(readback.errors.empty());
    assert(readback.warnings.empty());

    SearchTimerWorkflowValidationResult invalidReadback =
        service.validate(
            SearchTimerWorkflowRequest::readback(
                "livingroom",
                ""));

    assert(!invalidReadback.valid);
    assert(contains(
        invalidReadback.errors,
        "backendNativeId is required"));

    SearchTimerWorkflowValidationResult update =
        service.validate(
            SearchTimerWorkflowRequest::update(
                "livingroom",
                "42",
                "Daily News Updated",
                "Tagesschau Heute"));

    assert(update.valid);
    assert(!update.readOnly);
    assert(update.writeOperation);
    assert(update.wantsReadbackAfterWrite);
    assert(update.backendId == "livingroom");
    assert(update.backendNativeId == "42");
    assert(update.errors.empty());
    assert(contains(
        update.warnings,
        "write operation requires explicit operator intent"));
    assert(contains(
        update.warnings,
        "backend readback is recommended after this operation"));

    SearchTimerWorkflowValidationResult invalidUpdate =
        service.validate(
            SearchTimerWorkflowRequest::update(
                "livingroom",
                "",
                "Daily News Updated",
                "Tagesschau Heute"));

    assert(!invalidUpdate.valid);
    assert(contains(
        invalidUpdate.errors,
        "backendNativeId is required"));

    SearchTimerWorkflowValidationResult remove =
        service.validate(
            SearchTimerWorkflowRequest::remove(
                "livingroom",
                "42"));

    assert(remove.valid);
    assert(!remove.readOnly);
    assert(remove.writeOperation);
    assert(!remove.wantsReadbackAfterWrite);
    assert(remove.backendId == "livingroom");
    assert(remove.backendNativeId == "42");
    assert(remove.errors.empty());
    assert(contains(
        remove.warnings,
        "write operation requires explicit operator intent"));
    assert(!contains(
        remove.warnings,
        "backend readback is recommended after this operation"));

    SearchTimerWorkflowValidationResult invalidRemove =
        service.validate(
            SearchTimerWorkflowRequest::remove(
                "",
                "42"));

    assert(!invalidRemove.valid);
    assert(contains(invalidRemove.errors, "backendId is required"));

    std::cout << "test_search_timer_workflow_validation_service passed" << std::endl;
    return 0;
}
