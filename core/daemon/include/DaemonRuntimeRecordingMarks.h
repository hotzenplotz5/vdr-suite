#pragma once

#include "BackendRuntimeContext.h"
#include "VdrRecordingCacheRepository.h"

#include <memory>
#include <vector>

bool configureDaemonRecordingMarksRuntime(
    VdrRecordingCacheRepository& recordingCacheRepository,
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts);

void resetDaemonRecordingMarksRuntime();
