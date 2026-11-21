#pragma once

#include "BackendAccessPolicy.h"
#include "BackendRegistryService.h"
#include "RemoteActionDomain.h"

#include <map>
#include <memory>
#include <string>

class IRemoteActionExecutor
{
public:
    virtual ~IRemoteActionExecutor() = default;
    virtual RemoteActionResult execute(const RemoteActionRequest& request) const = 0;
};

class RemoteActionExecutorRegistry
{
public:
    void registerExecutor(
        const std::string& backendId,
        std::shared_ptr<IRemoteActionExecutor> executor);

    std::shared_ptr<IRemoteActionExecutor> findExecutor(
        const std::string& backendId) const;

    void clear();

private:
    std::map<std::string, std::shared_ptr<IRemoteActionExecutor>> executors_;
};

class RemoteActionService
{
public:
    RemoteActionService(
        BackendRegistryService& backendRegistryService,
        const BackendAccessPolicy& backendAccessPolicy,
        const RemoteActionExecutorRegistry& executorRegistry);

    RemoteActionResult execute(const RemoteActionRequest& request) const;

private:
    static RemoteActionResult validate(const RemoteActionRequest& request);

    BackendRegistryService& backendRegistryService_;
    const BackendAccessPolicy& backendAccessPolicy_;
    const RemoteActionExecutorRegistry& executorRegistry_;
};
