#pragma once

#include "DashboardController.h"

class BackendRegistryJsonSerializer;
class BackendRegistryService;

class BackendRegistryController
{
public:
    BackendRegistryController(
        BackendRegistryService& registryService,
        BackendRegistryJsonSerializer& jsonSerializer);

    ApiResponse getBackends();
    ApiResponse getDefaultBackend();

private:
    BackendRegistryService& registryService_;
    BackendRegistryJsonSerializer& jsonSerializer_;
};
