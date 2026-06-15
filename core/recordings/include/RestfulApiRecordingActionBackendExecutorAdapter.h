#pragma once

#include "IHttpClient.h"
#include "IRecordingActionBackendExecutorAdapter.h"
#include "RestfulApiRecordingActionBackendConfig.h"

#include <string>
#include <utility>

class RestfulApiRecordingActionBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    RestfulApiRecordingActionBackendExecutorAdapter(
        RestfulApiRecordingActionBackendConfig config,
        IHttpClient& httpClient)
        : config_(std::move(config)),
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
            "restfulapi backend executor adapter endpoint configuration only";
        return result;
    }

    std::string backendId() const override
    {
        return config_.backendId;
    }

    std::string backendType() const override
    {
        return "restfulapi";
    }

    const RestfulApiRecordingActionBackendConfig& config() const
    {
        return config_;
    }

private:
    RestfulApiRecordingActionBackendConfig config_;
    IHttpClient& httpClient_;
};
