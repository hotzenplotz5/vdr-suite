
#pragma once

#include "IVdrChannelMoveExecutor.h"

#include <memory>
#include <string>
#include <vector>

class IVdrChannelMoveExecutorAdapter
{
public:
    virtual ~IVdrChannelMoveExecutorAdapter() = default;

    virtual std::string backendId() const = 0;
    virtual IVdrChannelMoveExecutor& executor() = 0;
};

struct VdrChannelMoveExecutorAdapterLookupResult
{
    bool found = false;
    std::shared_ptr<IVdrChannelMoveExecutorAdapter> adapter;
    std::string backendId;
    std::string message;
};

class VdrChannelMoveExecutorAdapter final
    : public IVdrChannelMoveExecutorAdapter
{
public:
    VdrChannelMoveExecutorAdapter(
        std::string backendId,
        std::shared_ptr<IVdrChannelMoveExecutor> executor);

    std::string backendId() const override;
    IVdrChannelMoveExecutor& executor() override;

private:
    std::string backendId_;
    std::shared_ptr<IVdrChannelMoveExecutor> executor_;
};

class VdrChannelMoveExecutorAdapterRegistry
{
public:
    void registerAdapter(
        std::shared_ptr<IVdrChannelMoveExecutorAdapter> adapter);

    VdrChannelMoveExecutorAdapterLookupResult findAdapter(
        const std::string& backendId) const;

private:
    std::vector<std::shared_ptr<IVdrChannelMoveExecutorAdapter>> adapters_;
};
