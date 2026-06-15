#pragma once

#include "RecordingActionBackendExecutorAdapterLookupResult.h"
#include "RecordingActionDispatchResult.h"
#include "RecordingActionJobPayload.h"

class RecordingActionBackendExecutorAdapterDispatchService
{
public:
    RecordingActionDispatchResult dispatch(
        const RecordingActionBackendExecutorAdapterLookupResult& resolvedAdapter,
        const RecordingActionJobPayload& payload) const
    {
        RecordingActionDispatchResult result;

        if (!resolvedAdapter.found || !resolvedAdapter.adapter)
        {
            result.dispatched = false;
            result.reason = "no backend executor adapter resolved";
            return result;
        }

        result.dispatched = true;
        result.executionResult = resolvedAdapter.adapter->execute(payload);
        result.reason = "action dispatched to backend executor adapter";

        return result;
    }
};
