#pragma once

#include "RecordingActionDefaultExecutorResolutionResult.h"
#include "RecordingActionExecutorLookupResult.h"
#include "RecordingActionExecutorRegistry.h"
#include "RecordingActionExecutorSelectionResult.h"

class RecordingActionExecutorResolutionService
{
public:
    RecordingActionExecutorSelectionResult resolve(
        const RecordingActionExecutorLookupResult& lookupResult,
        const RecordingActionDefaultExecutorResolutionResult& defaultResolution) const
    {
        RecordingActionExecutorSelectionResult result;

        if (lookupResult.found)
        {
            result.selected = true;
            result.backendId = lookupResult.backendId;
            result.executor = lookupResult.executor;
            result.reason = "resolved by lookup";
            return result;
        }

        if (defaultResolution.resolved)
        {
            result.selected = true;
            result.backendId = defaultResolution.backendId;
            result.executor = defaultResolution.executor;
            result.reason = "resolved by default executor";
            return result;
        }

        result.selected = false;
        result.reason = "no executor available";
        return result;
    }
};
