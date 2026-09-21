#include "RestfulApiVdrTimerActionExecutorAdapter.h"

RestfulApiVdrTimerActionExecutorAdapter::RestfulApiVdrTimerActionExecutorAdapter(
    std::string backendId,
    std::string basePath,
    IHttpClient& httpClient)
    : backendId_(backendId),
      executor_(
          backendId,
          basePath,
          httpClient)
{
}

std::string RestfulApiVdrTimerActionExecutorAdapter::backendId() const
{
    return backendId_;
}

IVdrTimerActionExecutor& RestfulApiVdrTimerActionExecutorAdapter::executor()
{
    return executor_;
}
