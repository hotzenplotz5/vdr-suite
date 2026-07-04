
#include "VdrChannelMoveExecutorAdapterRegistry.h"

#include <utility>

VdrChannelMoveExecutorAdapter::VdrChannelMoveExecutorAdapter(
    std::string backendId,
    std::shared_ptr<IVdrChannelMoveExecutor> executor)
    : backendId_(std::move(backendId)),
      executor_(std::move(executor))
{
}

std::string VdrChannelMoveExecutorAdapter::backendId() const
{
    return backendId_;
}

IVdrChannelMoveExecutor& VdrChannelMoveExecutorAdapter::executor()
{
    return *executor_;
}

void VdrChannelMoveExecutorAdapterRegistry::registerAdapter(
    std::shared_ptr<IVdrChannelMoveExecutorAdapter> adapter)
{
    if (!adapter)
    {
        return;
    }

    for (auto& existingAdapter : adapters_)
    {
        if (existingAdapter &&
            existingAdapter->backendId() == adapter->backendId())
        {
            existingAdapter = adapter;
            return;
        }
    }

    adapters_.push_back(std::move(adapter));
}

VdrChannelMoveExecutorAdapterLookupResult
VdrChannelMoveExecutorAdapterRegistry::findAdapter(
    const std::string& backendId) const
{
    VdrChannelMoveExecutorAdapterLookupResult result;
    result.backendId = backendId;

    for (const auto& adapter : adapters_)
    {
        if (adapter &&
            adapter->backendId() == backendId)
        {
            result.found = true;
            result.adapter = adapter;
            result.message = "channel move executor found";
            return result;
        }
    }

    result.message =
        "no channel move executor registered for backend \"" +
        backendId +
        "\"";
    return result;
}
