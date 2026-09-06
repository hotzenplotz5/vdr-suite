#pragma once

#include "BackendRuntimeContext.h"
#include "VdrRecordingCacheRepository.h"

#include <memory>
#include <vector>

class BackendAccessPolicy;
class BackendAgentCommandRepository;
class BackendAgentRepository;
class BackendRegistryService;

bool configureDaemonRecordingCutRuntime(
    VdrRecordingCacheRepository& recordingCacheRepository,
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts,
    BackendRegistryService& backendRegistryService,
    BackendAccessPolicy& backendAccessPolicy,
    BackendAgentRepository& backendAgentRepository,
    BackendAgentCommandRepository& backendAgentCommandRepository);

void resetDaemonRecordingCutRuntime();
