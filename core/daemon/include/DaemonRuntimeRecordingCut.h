#pragma once

#include "BackendRuntimeContext.h"
#include "VdrRecordingCacheRepository.h"

#include <memory>
#include <string>
#include <vector>

class BackendAccessPolicy;
class BackendAgentCommandRepository;
class BackendAgentRepository;
class BackendRegistryService;

std::string daemonRecordingCutDeleteBlockReason(
    VdrRecordingCacheRepository& recordingCacheRepository,
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts,
    const std::string& backendId,
    const std::string& recordingId);

bool configureDaemonRecordingCutRuntime(
    VdrRecordingCacheRepository& recordingCacheRepository,
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts,
    BackendRegistryService& backendRegistryService,
    BackendAccessPolicy& backendAccessPolicy,
    BackendAgentRepository& backendAgentRepository,
    BackendAgentCommandRepository& backendAgentCommandRepository);

void resetDaemonRecordingCutRuntime();
