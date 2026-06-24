#include "SearchTimerWorkflowValidationService.h"

SearchTimerWorkflowValidationResult SearchTimerWorkflowValidationService::validate(
    const SearchTimerWorkflowRequest& request) const
{
    SearchTimerWorkflowValidationResult result;

    result.operation = request.operation();
    result.readOnly = request.isReadOnly();
    result.writeOperation = request.isWriteOperation();
    result.wantsReadbackAfterWrite =
        request.wantsReadbackAfterWrite();
    result.backendId = request.backendId();
    result.backendNativeId = request.backendNativeId();

    if (!request.hasOperation())
    {
        result.errors.push_back("searchtimer workflow operation is required");
    }

    if (request.requiresBackend() && !request.hasBackend())
    {
        result.errors.push_back("backendId is required");
    }

    if (request.requiresBackendNativeId() &&
        !request.hasBackendNativeId())
    {
        result.errors.push_back("backendNativeId is required");
    }

    if (request.requiresName() && !request.hasName())
    {
        result.errors.push_back("name is required");
    }

    if (request.requiresQuery() && !request.hasQuery())
    {
        result.errors.push_back("query is required");
    }

    if (request.isWriteOperation())
    {
        result.warnings.push_back(
            "write operation requires explicit operator intent");
    }

    if (request.wantsReadbackAfterWrite())
    {
        result.warnings.push_back(
            "backend readback is recommended after this operation");
    }

    result.valid =
        result.errors.empty();

    return result;
}
