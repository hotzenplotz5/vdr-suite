#pragma once

#include "IRecordingActionExecutor.h"
#include "RecordingActionExecutorRegistration.h"
#include "RecordingActionExecutorLookupResult.h"

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

    RecordingActionExecutorLookupResult findExecutor(
        const std::string& backendId) const
    {
        auto iterator = executors_.find(backendId);

        if (iterator == executors_.end())
        {
            RecordingActionExecutorLookupResult result;
            result.found = false;
            result.backendId = backendId;
            result.message = "executor not found";
            return result;
        }

        RecordingActionExecutorLookupResult result;
        result.found = true;
        result.backendId = backendId;
        result.executor = iterator->second;
        result.message = "executor found";
        return result;
    }

private:
    std::map<std::string, std::shared_ptr<IRecordingActionExecutor>> executors_;
};
