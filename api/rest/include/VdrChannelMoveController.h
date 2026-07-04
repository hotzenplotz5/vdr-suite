
#pragma once

#include "BackendAccessPolicy.h"
#include "DashboardController.h"
#include "VdrChannelMoveExecutionService.h"
#include "VdrChannelMoveExecutorAdapterRegistry.h"
#include "VdrChannelMoveRequestParser.h"
#include "VdrChannelMoveResultJsonSerializer.h"

#include <functional>
#include <string>

class VdrChannelMoveController
{
public:
    using AfterSuccessfulMoveCallback =
        std::function<void(const std::string& backendId)>;

    VdrChannelMoveController(
        VdrChannelMoveExecutionService& executionService,
        VdrChannelMoveResultJsonSerializer& jsonSerializer,
        VdrChannelMoveRequestParser& requestParser,
        const VdrChannelMoveExecutorAdapterRegistry& registry,
        const BackendRegistryService& backendRegistryService,
        const BackendAccessPolicy& backendAccessPolicy);

    ApiResponse moveBody(
        const std::string& body);

    void setAfterSuccessfulMoveCallback(
        AfterSuccessfulMoveCallback callback);

private:
    VdrChannelMoveExecutionService& executionService_;
    VdrChannelMoveResultJsonSerializer& jsonSerializer_;
    VdrChannelMoveRequestParser& requestParser_;
    const VdrChannelMoveExecutorAdapterRegistry& registry_;
    const BackendRegistryService& backendRegistryService_;
    const BackendAccessPolicy& backendAccessPolicy_;
    AfterSuccessfulMoveCallback afterSuccessfulMoveCallback_;
};
