#pragma once

#include "BackendRuntimeContext.h"

#include <memory>
#include <vector>

class BackendAgentCommandRepository;

bool configureDaemonRecordingCutRuntime(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts,
    BackendAgentCommandRepository& backendAgentCommandRepository);

void resetDaemonRecordingCutRuntime();
