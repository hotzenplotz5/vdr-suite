#pragma once

#include "IRecordingActionBackendExecutorAdapter.h"

#include <map>
#include <memory>
#include <string>

class RecordingActionBackendExecutorAdapterRegistry
{
public:
    void registerAdapter(
        std::shared_ptr<IRecordingActionBackendExecutorAdapter> adapter)
    {
        if (!adapter)
        {
            return;
        }

        adapters_[adapter->backendId()] = adapter;
    }

    std::shared_ptr<IRecordingActionBackendExecutorAdapter> findAdapter(
        const std::string& backendId) const
    {
        auto iterator = adapters_.find(backendId);

        if (iterator == adapters_.end())
        {
            return nullptr;
        }

        return iterator->second;
    }

private:
    std::map<
        std::string,
        std::shared_ptr<IRecordingActionBackendExecutorAdapter>
    > adapters_;
};
