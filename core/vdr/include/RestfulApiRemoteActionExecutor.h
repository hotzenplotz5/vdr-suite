#pragma once

#include "IHttpClient.h"
#include "RemoteActionService.h"

#include <string>

class RestfulApiRemoteActionExecutor : public IRemoteActionExecutor
{
public:
    RestfulApiRemoteActionExecutor(
        std::string backendId,
        std::string basePath,
        IHttpClient& httpClient);

    RemoteActionResult execute(
        const RemoteActionRequest& request) const override;

    static std::string endpointForAction(RemoteActionType action);
    static std::string percentEncodePathSegment(const std::string& value);

private:
    static std::string buildUrl(
        const std::string& basePath,
        const std::string& endpoint);

    std::string backendId_;
    std::string basePath_;
    IHttpClient& httpClient_;
};
