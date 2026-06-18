#pragma once

#include "IVdrTimerActionExecutorAdapter.h"
#include "RestfulApiVdrTimerActionExecutor.h"

#include <string>

class IHttpClient;

class RestfulApiVdrTimerActionExecutorAdapter final : public IVdrTimerActionExecutorAdapter
{
public:
    RestfulApiVdrTimerActionExecutorAdapter(
        std::string backendId,
        std::string basePath,
        IHttpClient& httpClient);

    std::string backendId() const override;

    IVdrTimerActionExecutor& executor() override;

private:
    std::string backendId_;
    RestfulApiVdrTimerActionExecutor executor_;
};
