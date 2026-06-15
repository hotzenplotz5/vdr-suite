#pragma once

#include "IRecordingActionExecutor.h"

#include <map>
#include <memory>
#include <string>

class RecordingActionExecutorRegistry
{
public:
    void registerExecutor(
        const std::string& backendId,
        std::shared_ptr<IRecordingActionExecutor> executor)
    {
        executors_[backendId] = executor;
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
