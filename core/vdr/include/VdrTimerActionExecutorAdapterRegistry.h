#pragma once

#include "IVdrTimerActionExecutorAdapter.h"
#include "VdrTimerActionExecutorAdapterLookupResult.h"

#include <map>
#include <memory>
#include <string>

class VdrTimerActionExecutorAdapterRegistry
{
public:
    void registerAdapter(
        std::shared_ptr<IVdrTimerActionExecutorAdapter> adapter)
    {
        if (!adapter)
        {
            return;
        }

        adapters_[adapter->backendId()] = adapter;
    }

    VdrTimerActionExecutorAdapterLookupResult findAdapter(
        const std::string& backendId) const
    {
        auto iterator = adapters_.find(backendId);

        if (iterator == adapters_.end())
        {
            VdrTimerActionExecutorAdapterLookupResult result;
            result.found = false;
            result.backendId = backendId;
            result.message = "timer action executor adapter not found";
            return result;
        }

        VdrTimerActionExecutorAdapterLookupResult result;
        result.found = true;
        result.backendId = backendId;
        result.adapter = iterator->second;
        result.message = "timer action executor adapter found";
        return result;
    }

private:
    std::map<
        std::string,
        std::shared_ptr<IVdrTimerActionExecutorAdapter>
    > adapters_;
};
