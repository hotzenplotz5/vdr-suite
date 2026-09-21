#include "LiveRemoteApiRuntime.h"

#include "LiveOverlayController.h"
#include "RemoteActionController.h"
#include "RemoteActionRequestParser.h"
#include "RestfulApiLiveChannelStateProvider.h"
#include "RestfulApiRemoteActionExecutor.h"
#include "SnapshotCacheService.h"
#include "VdrSnapshotReadService.h"

#include <cctype>
#include <memory>
#include <utility>

LiveRemoteApiRuntime& LiveRemoteApiRuntime::instance()
{
    static LiveRemoteApiRuntime runtime;
    return runtime;
}

void LiveRemoteApiRuntime::configure(
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    SnapshotCacheService& snapshotCacheService,
    ChangePublisher changePublisher,
    LiveOverlayService::EventLookup eventLookup)
{
    backendRegistryService_ = &backendRegistryService;
    snapshotReadService_ = &snapshotReadService;
    snapshotCacheService_ = &snapshotCacheService;
    changePublisher_ = std::move(changePublisher);
    eventLookup_ = std::move(eventLookup);
}

void LiveRemoteApiRuntime::registerRestfulApiBackend(
    const std::string& backendId,
    IHttpClient& httpClient)
{
    remoteActionExecutorRegistry_.registerExecutor(
        backendId,
        std::make_shared<RestfulApiRemoteActionExecutor>(
            backendId,
            "",
            httpClient));

    liveChannelStateProviderRegistry_.registerProvider(
        backendId,
        std::make_shared<RestfulApiLiveChannelStateProvider>(httpClient));
}

void LiveRemoteApiRuntime::reset()
{
    backendRegistryService_ = nullptr;
    snapshotReadService_ = nullptr;
    snapshotCacheService_ = nullptr;
    changePublisher_ = {};
    eventLookup_ = {};
    remoteActionExecutorRegistry_.clear();
    liveChannelStateProviderRegistry_.clear();
}

bool LiveRemoteApiRuntime::configured() const
{
    return backendRegistryService_ != nullptr &&
        snapshotReadService_ != nullptr &&
        snapshotCacheService_ != nullptr;
}

std::string LiveRemoteApiRuntime::pathPart(
    const std::string& requestTarget)
{
    const std::size_t query = requestTarget.find('?');
    return requestTarget.substr(0, query);
}

std::string LiveRemoteApiRuntime::percentDecode(
    const std::string& value)
{
    std::string decoded;
    decoded.reserve(value.size());

    auto hex = [](char character) -> int
    {
        if (character >= '0' && character <= '9') return character - '0';
        if (character >= 'a' && character <= 'f') return 10 + character - 'a';
        if (character >= 'A' && character <= 'F') return 10 + character - 'A';
        return -1;
    };

    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == '%' && index + 2 < value.size())
        {
            const int high = hex(value[index + 1]);
            const int low = hex(value[index + 2]);

            if (high >= 0 && low >= 0)
            {
                decoded.push_back(static_cast<char>((high * 16) + low));
                index += 2;
                continue;
            }
        }

        decoded.push_back(value[index] == '+' ? ' ' : value[index]);
    }

    return decoded;
}

std::string LiveRemoteApiRuntime::queryValue(
    const std::string& requestTarget,
    const std::string& name)
{
    const std::size_t query = requestTarget.find('?');

    if (query == std::string::npos)
    {
        return "";
    }

    std::size_t position = query + 1;

    while (position <= requestTarget.size())
    {
        const std::size_t separator = requestTarget.find('&', position);
        const std::string item = requestTarget.substr(
            position,
            separator == std::string::npos
                ? std::string::npos
                : separator - position);
        const std::size_t equals = item.find('=');
        const std::string key = percentDecode(item.substr(0, equals));

        if (key == name)
        {
            return equals == std::string::npos
                ? ""
                : percentDecode(item.substr(equals + 1));
        }

        if (separator == std::string::npos)
        {
            break;
        }

        position = separator + 1;
    }

    return "";
}

bool LiveRemoteApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& body,
    ApiResponse& response) const
{
    if (pathPart(requestTarget) != "/api/vdr/remote/actions")
    {
        return false;
    }

    if (!configured())
    {
        response.statusCode = 503;
        response.contentType = "application/json";
        response.body = "{\"success\":false,\"failureKind\":\"executorUnavailable\",\"message\":\"live remote runtime is not configured\"}";
        return true;
    }

    const RemoteActionRequestParser parser;
    const RemoteActionResultJsonSerializer serializer;
    const RemoteActionService service(
        *backendRegistryService_,
        backendAccessPolicy_,
        remoteActionExecutorRegistry_);
    const RemoteActionController controller(
        parser,
        service,
        serializer,
        [this](const RemoteActionRequest& request)
        {
            if (changePublisher_)
            {
                changePublisher_(request.backendId);
            }
        });

    response = controller.executeBody(body);
    return true;
}

bool LiveRemoteApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    if (pathPart(requestTarget) != "/api/vdr/live/overlay")
    {
        return false;
    }

    if (!configured())
    {
        response.statusCode = 503;
        response.contentType = "application/json";
        response.body = "{\"success\":false,\"message\":\"live remote runtime is not configured\"}";
        return true;
    }

    std::string backendId = queryValue(requestTarget, "backend");

    if (backendId.empty())
    {
        backendId = queryValue(requestTarget, "backendId");
    }

    if (backendId.empty())
    {
        backendId = "default";
    }

    const LiveOverlayService service(
        *backendRegistryService_,
        *snapshotReadService_,
        *snapshotCacheService_,
        liveChannelStateProviderRegistry_,
        eventLookup_);
    const LiveOverlaySnapshotJsonSerializer serializer;
    const LiveOverlayController controller(service, serializer);
    response = controller.getSnapshot(backendId);
    return true;
}
