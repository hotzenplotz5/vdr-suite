#pragma once

#include "IRecordingActionBackendExecutorAdapter.h"

#include <memory>
#include <string>

struct RecordingActionBackendExecutorAdapterLookupResult
{
    bool found = false;

    std::string backendId;

    std::shared_ptr<IRecordingActionBackendExecutorAdapter> adapter;

    std::string message;
};
