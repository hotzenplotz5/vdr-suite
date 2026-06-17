#pragma once

#include "IHttpClient.h"
#include "IRecordingActionBackendExecutorAdapter.h"
#include "RecordingActionCapabilityContract.h"
#include "RestfulApiRecordingActionRequestBuilder.h"

class RestfulApiRecordingActionExecutor final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    RestfulApiRecordingActionExecutor(
        std::string backendId,
        std::string backendType,
        RestfulApiRecordingActionBackendConfig config,
        IHttpClient& httpClient);

    std::string backendId() const override;
    std::string backendType() const override;
    RecordingActionCapabilitySet capabilities() const override;

    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override;

private:
    RecordingActionExecutionResult executeBuiltRequest(
        const RecordingActionJobPayload& payload,
        const HttpRequest& request) const;

    std::string backendId_;
    std::string backendType_;
    RestfulApiRecordingActionBackendConfig config_;
    IHttpClient& httpClient_;
    RestfulApiRecordingActionRequestBuilder requestBuilder_;
};
