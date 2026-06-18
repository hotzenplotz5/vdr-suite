#pragma once

#include "IHttpClient.h"
#include "IVdrTimerActionExecutor.h"
#include "RestfulApiVdrTimerActionRequestBuilder.h"

#include <string>

class RestfulApiVdrTimerActionExecutor final : public IVdrTimerActionExecutor
{
public:
    RestfulApiVdrTimerActionExecutor(
        std::string backendId,
        std::string basePath,
        IHttpClient& httpClient);

    VdrTimerActionResult execute(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request) override;

private:
    VdrTimerActionResult executeBuiltRequest(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request,
        const HttpRequest& httpRequest) const;

    std::string backendId_;
    std::string basePath_;
    IHttpClient& httpClient_;
    RestfulApiVdrTimerActionRequestBuilder requestBuilder_;
};
