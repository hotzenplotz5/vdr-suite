#pragma once

#include "RecordingActionBackendExecutorAdapterLookupResult.h"

class RecordingActionBackendExecutorAdapterResolutionService
{
public:
    RecordingActionBackendExecutorAdapterLookupResult resolve(
        const RecordingActionBackendExecutorAdapterLookupResult& lookupResult) const
    {
        if (lookupResult.found)
        {
            return lookupResult;
        }

        RecordingActionBackendExecutorAdapterLookupResult result;
        result.found = false;
        result.backendId = lookupResult.backendId;
        result.message = "backend executor adapter resolution failed";
        return result;
    }
};
