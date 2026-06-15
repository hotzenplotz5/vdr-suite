#pragma once

#include "IRecordingActionExecutor.h"
#include "RecordingActionExecutorRegistration.h"

#include <map>
#include <memory>
#include <string>

class RecordingActionExecutorRegistry
{
public:
    void registerExecutor(
        const RecordingActionExecutorRegistration& registration)
    {
        executors_[registration.backendId] =
            registration.executor;
    }

    std::shared_ptr<IRecordingActionExecutor> findExecutor(
        const std::string& backendId) const
    {
        auto iterator = executors_.find(backendId);

        if (iterator == executors_.end())
        {
            return nullptr;
        }

        return iterator->second;
    }

private:
    std::map<std::string, std::shared_ptr<IRecordingActionExecutor>> executors_;
};
