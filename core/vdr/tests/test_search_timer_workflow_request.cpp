#include "SearchTimerWorkflowRequest.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerWorkflowRequest listAll =
        SearchTimerWorkflowRequest::list();

    assert(listAll.operation() == SearchTimerWorkflowOperation::List);
    assert(listAll.hasOperation());
    assert(!listAll.hasBackend());
    assert(listAll.isReadOnly());
    assert(!listAll.isWriteOperation());
    assert(!listAll.requiresBackend());
    assert(!listAll.requiresBackendNativeId());
    assert(!listAll.wantsReadbackAfterWrite());
    assert(listAll.isValid());

    SearchTimerWorkflowRequest listBackend =
        SearchTimerWorkflowRequest::list("livingroom");

    assert(listBackend.hasBackend());
    assert(listBackend.backendId() == "livingroom");
    assert(listBackend.isValid());

    SearchTimerWorkflowRequest preview =
        SearchTimerWorkflowRequest::preview(
            "livingroom",
            "Preview SearchTimer",
            "Tatort");

    assert(preview.operation() == SearchTimerWorkflowOperation::Preview);
    assert(preview.isReadOnly());
    assert(!preview.isWriteOperation());
    assert(preview.requiresBackend());
    assert(!preview.requiresBackendNativeId());
    assert(preview.requiresName());
    assert(preview.requiresQuery());
    assert(preview.backendId() == "livingroom");
    assert(preview.name() == "Preview SearchTimer");
    assert(preview.query() == "Tatort");
    assert(preview.isValid());

    SearchTimerWorkflowRequest invalidPreview =
        SearchTimerWorkflowRequest::preview(
            "",
            "Preview SearchTimer",
            "Tatort");

    assert(invalidPreview.requiresBackend());
    assert(!invalidPreview.isValid());

    SearchTimerWorkflowRequest create =
        SearchTimerWorkflowRequest::create(
            "livingroom",
            "Daily News",
            "Tagesschau");

    assert(create.operation() == SearchTimerWorkflowOperation::Create);
    assert(!create.isReadOnly());
    assert(create.isWriteOperation());
    assert(create.requiresBackend());
    assert(!create.requiresBackendNativeId());
    assert(create.requiresName());
    assert(create.requiresQuery());
    assert(create.active());
    assert(create.wantsReadbackAfterWrite());
    assert(create.isValid());


    SearchTimerWorkflowRequest titleOnlyWorkflowCreate =
        SearchTimerWorkflowRequest::create(
            "livingroom",
            "Amerika",
            "Amerika")
            .withCompareFields(
                true,
                false,
                false,
                false);

    assert(titleOnlyWorkflowCreate.operation() == SearchTimerWorkflowOperation::Create);
    assert(titleOnlyWorkflowCreate.compareTitle());
    assert(!titleOnlyWorkflowCreate.compareSubtitle());
    assert(!titleOnlyWorkflowCreate.compareSummary());
    assert(!titleOnlyWorkflowCreate.compareCategories());
    assert(titleOnlyWorkflowCreate.isValid());

    SearchTimerWorkflowRequest inactiveCreate =
        SearchTimerWorkflowRequest::create(
            "livingroom",
            "Dormant Search",
            "Archive",
            false);

    assert(!inactiveCreate.active());
    assert(inactiveCreate.isValid());

    SearchTimerWorkflowRequest readback =
        SearchTimerWorkflowRequest::readback(
            "livingroom",
            "42");

    assert(readback.operation() == SearchTimerWorkflowOperation::Readback);
    assert(readback.isReadOnly());
    assert(!readback.isWriteOperation());
    assert(readback.requiresBackend());
    assert(readback.requiresBackendNativeId());
    assert(!readback.requiresName());
    assert(!readback.requiresQuery());
    assert(readback.backendNativeId() == "42");
    assert(!readback.wantsReadbackAfterWrite());
    assert(readback.isValid());

    SearchTimerWorkflowRequest update =
        SearchTimerWorkflowRequest::update(
            "livingroom",
            "42",
            "Daily News Updated",
            "Tagesschau Heute",
            false);

    assert(update.operation() == SearchTimerWorkflowOperation::Update);
    assert(!update.isReadOnly());
    assert(update.isWriteOperation());
    assert(update.requiresBackend());
    assert(update.requiresBackendNativeId());
    assert(update.requiresName());
    assert(update.requiresQuery());
    assert(update.backendNativeId() == "42");
    assert(update.name() == "Daily News Updated");
    assert(update.query() == "Tagesschau Heute");
    assert(!update.active());
    assert(update.wantsReadbackAfterWrite());
    assert(update.isValid());


    SearchTimerWorkflowRequest titleOnlyWorkflowUpdate =
        SearchTimerWorkflowRequest::update(
            "livingroom",
            "searchtimer-amerika",
            "Amerika",
            "Amerika")
            .withCompareFields(
                true,
                false,
                false,
                false);

    assert(titleOnlyWorkflowUpdate.operation() == SearchTimerWorkflowOperation::Update);
    assert(titleOnlyWorkflowUpdate.backendNativeId() == "searchtimer-amerika");
    assert(titleOnlyWorkflowUpdate.compareTitle());
    assert(!titleOnlyWorkflowUpdate.compareSubtitle());
    assert(!titleOnlyWorkflowUpdate.compareSummary());
    assert(!titleOnlyWorkflowUpdate.compareCategories());
    assert(titleOnlyWorkflowUpdate.isValid());

    SearchTimerWorkflowRequest invalidUpdate =
        SearchTimerWorkflowRequest::update(
            "livingroom",
            "",
            "Daily News Updated",
            "Tagesschau Heute");

    assert(invalidUpdate.requiresBackendNativeId());
    assert(!invalidUpdate.isValid());

    SearchTimerWorkflowRequest remove =
        SearchTimerWorkflowRequest::remove(
            "livingroom",
            "42");

    assert(remove.operation() == SearchTimerWorkflowOperation::Delete);
    assert(!remove.isReadOnly());
    assert(remove.isWriteOperation());
    assert(remove.requiresBackend());
    assert(remove.requiresBackendNativeId());
    assert(!remove.requiresName());
    assert(!remove.requiresQuery());
    assert(remove.backendId() == "livingroom");
    assert(remove.backendNativeId() == "42");
    assert(!remove.wantsReadbackAfterWrite());
    assert(remove.isValid());

    SearchTimerWorkflowRequest invalidRemove =
        SearchTimerWorkflowRequest::remove(
            "",
            "42");

    assert(invalidRemove.requiresBackend());
    assert(!invalidRemove.isValid());

    std::cout << "test_search_timer_workflow_request passed" << std::endl;
    return 0;
}
