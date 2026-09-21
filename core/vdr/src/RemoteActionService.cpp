#include "RemoteActionService.h"

void RemoteActionExecutorRegistry::registerExecutor(
    const std::string& backendId,
    std::shared_ptr<IRemoteActionExecutor> executor)
{
    if (backendId.empty() || !executor)
    {
        return;
    }

    executors_[backendId] = std::move(executor);
}

std::shared_ptr<IRemoteActionExecutor> RemoteActionExecutorRegistry::findExecutor(
    const std::string& backendId) const
{
    const auto iterator = executors_.find(backendId);
    return iterator == executors_.end() ? nullptr : iterator->second;
}

void RemoteActionExecutorRegistry::clear()
{
    executors_.clear();
}

RemoteActionService::RemoteActionService(
    BackendRegistryService& backendRegistryService,
    const BackendAccessPolicy& backendAccessPolicy,
    const RemoteActionExecutorRegistry& executorRegistry)
    : backendRegistryService_(backendRegistryService),
      backendAccessPolicy_(backendAccessPolicy),
      executorRegistry_(executorRegistry)
{
}

RemoteActionResult RemoteActionService::validate(
    const RemoteActionRequest& request)
{
    if (!isRemoteActionRequestToken(request.backendId, 128))
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::Validation,
            "Remote action request is invalid",
            {"backendId is required and must use a safe token"});
    }

    if (!isRemoteActionRequestToken(request.operationId, 128))
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::Validation,
            "Remote action request is invalid",
            {"operationId is required and must use a safe token"});
    }

    if (!isRemoteActionAllowlisted(request.action))
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::Validation,
            "Remote action is not allowlisted",
            {"unknown or disallowed action"});
    }

    if (request.action == RemoteActionType::SwitchChannel)
    {
        if (request.channelId.empty() || request.channelId.size() > 256)
        {
            return RemoteActionResult::failed(
                request,
                RemoteActionFailureKind::Validation,
                "Remote channel switch request is invalid",
                {"channelId is required for switchChannel"});
        }
    }
    else if (!request.channelId.empty())
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::Validation,
            "Remote action request is invalid",
            {"channelId is only valid for switchChannel"});
    }

    return RemoteActionResult::ok(request, "Remote action request is valid");
}

RemoteActionResult RemoteActionService::execute(
    const RemoteActionRequest& request) const
{
    const RemoteActionResult validation = validate(request);

    if (!validation.success)
    {
        return validation;
    }

    const auto backend = backendRegistryService_.getBackend(request.backendId);

    if (!backend.has_value())
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::BackendNotFound,
            "Remote action backend was not found",
            {"backend not found: " + request.backendId});
    }

    const BackendAccessDecision access =
        backendAccessPolicy_.canWriteToBackend(
            backendRegistryService_,
            request.backendId);

    if (!access.allowed)
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::Permission,
            "Remote action is not permitted for this backend",
            access.errors);
    }

    if (!backend->capabilities.remoteControl)
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::Capability,
            "Backend does not expose the remote.control capability",
            {"remote.control capability is unavailable"});
    }

    const auto executor = executorRegistry_.findExecutor(request.backendId);

    if (!executor)
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::ExecutorUnavailable,
            "Remote action executor is unavailable",
            {"executor not registered for backend: " + request.backendId});
    }

    return executor->execute(request);
}
