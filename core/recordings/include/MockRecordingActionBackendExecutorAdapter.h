#pragma once

#include "IRecordingActionBackendExecutorAdapter.h"

class MockRecordingActionBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override
    {
        RecordingActionExecutionResult result;
        result.type = payload.type;
        result.success = true;
        result.backendId = backendId();
        result.recordingId = payload.recordingId;
        result.message = "mock backend executor accepted payload";
        return result;
    }

    std::string backendId() const override
    {
        return "mock";
    }

    std::string backendType() const override
    {
        return "mock";
    }
};
