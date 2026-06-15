#pragma once

#include "IRecordingActionBackendExecutorAdapter.h"
#include "RecordingActionBackendExecutorAdapterLookupResult.h"

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

    RecordingActionBackendExecutorAdapterLookupResult findAdapter(
        const std::string& backendId) const
    {
        auto iterator = adapters_.find(backendId);

        if (iterator == adapters_.end())
        {
            RecordingActionBackendExecutorAdapterLookupResult result;
            result.found = false;
            result.backendId = backendId;
            result.message = "backend executor adapter not found";
            return result;
        }

        RecordingActionBackendExecutorAdapterLookupResult result;
        result.found = true;
        result.backendId = backendId;
        result.adapter = iterator->second;
        result.message = "backend executor adapter found";
        return result;
    }

private:
    std::map<
        std::string,
        std::shared_ptr<IRecordingActionBackendExecutorAdapter>
    > adapters_;
};
