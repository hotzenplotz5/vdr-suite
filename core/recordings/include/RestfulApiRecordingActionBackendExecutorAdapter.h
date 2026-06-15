#pragma once

#include "IHttpClient.h"
#include "IRecordingActionBackendExecutorAdapter.h"

#include <string>
#include <utility>

class RestfulApiRecordingActionBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    RestfulApiRecordingActionBackendExecutorAdapter(
        std::string backendId,
        IHttpClient& httpClient)
        : backendId_(std::move(backendId)),
          httpClient_(httpClient)
    {
    }

    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override
    {
        RecordingActionExecutionResult result;
        result.type = payload.type;
        result.success = false;
        result.backendId = backendId();
        result.recordingId = payload.recordingId;
        result.message =
            "restfulapi backend executor adapter foundation only";
        return result;
    }

    std::string backendId() const override
    {
        return backendId_;
    }

    std::string backendType() const override
    {
        return "restfulapi";
    }

private:
    std::string backendId_;
    IHttpClient& httpClient_;
};
