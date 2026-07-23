#include "MockHttpClient.h"
#include "RestfulApiRemoteActionExecutor.h"

#include <cassert>
#include <map>
#include <stdexcept>

class ThrowingHttpClient : public IHttpClient
{
public:
    HttpResponse execute(const HttpRequest&) const override
    {
        throw std::runtime_error("transport down");
    }
};

namespace
{
RemoteActionRequest request(RemoteActionType action)
{
    RemoteActionRequest value;
    value.backendId = "default";
    value.operationId = "remote-test";
    value.action = action;
    return value;
}
}

int main()
{
    MockHttpClient httpClient;
    HttpResponse response;
    response.statusCode = 204;
    httpClient.setResponse(response);

    RestfulApiRemoteActionExecutor executor("default", "", httpClient);

    const std::map<RemoteActionType, std::string> expectedEndpoints = {
        {RemoteActionType::Up, "/remote/up"},
        {RemoteActionType::Down, "/remote/down"},
        {RemoteActionType::Left, "/remote/left"},
        {RemoteActionType::Right, "/remote/right"},
        {RemoteActionType::Ok, "/remote/ok"},
        {RemoteActionType::Back, "/remote/back"},
        {RemoteActionType::Menu, "/remote/menu"},
        {RemoteActionType::Info, "/remote/info"},
        {RemoteActionType::Red, "/remote/red"},
        {RemoteActionType::Green, "/remote/green"},
        {RemoteActionType::Yellow, "/remote/yellow"},
        {RemoteActionType::Blue, "/remote/blue"},
        {RemoteActionType::Zero, "/remote/0"},
        {RemoteActionType::One, "/remote/1"},
        {RemoteActionType::Two, "/remote/2"},
        {RemoteActionType::Three, "/remote/3"},
        {RemoteActionType::Four, "/remote/4"},
        {RemoteActionType::Five, "/remote/5"},
        {RemoteActionType::Six, "/remote/6"},
        {RemoteActionType::Seven, "/remote/7"},
        {RemoteActionType::Eight, "/remote/8"},
        {RemoteActionType::Nine, "/remote/9"},
        {RemoteActionType::ChannelUp, "/remote/chanup"},
        {RemoteActionType::ChannelDown, "/remote/chandn"},
        {RemoteActionType::VolumeUp, "/remote/volup"},
        {RemoteActionType::VolumeDown, "/remote/voldn"},
        {RemoteActionType::Mute, "/remote/mute"},
        {RemoteActionType::Play, "/remote/play"},
        {RemoteActionType::Pause, "/remote/pause"},
        {RemoteActionType::Stop, "/remote/stop"},
        {RemoteActionType::Record, "/remote/record"},
        {RemoteActionType::FastForward, "/remote/fastfwd"},
        {RemoteActionType::Rewind, "/remote/fastrew"},
        {RemoteActionType::Next, "/remote/next"},
        {RemoteActionType::Previous, "/remote/prev"}
    };

    for (const auto& entry : expectedEndpoints)
    {
        const RemoteActionResult result = executor.execute(request(entry.first));
        assert(result.success);
        assert(httpClient.lastRequest().method == "POST");
        assert(httpClient.lastRequest().url == entry.second);
    }

    RemoteActionRequest channelSwitch = request(RemoteActionType::SwitchChannel);
    channelSwitch.channelId = "C-1:2/3 ä";
    RemoteActionResult result = executor.execute(channelSwitch);
    assert(result.success);
    assert(httpClient.lastRequest().url == "/remote/switch/C-1%3A2%2F3%20%C3%A4");

    response.statusCode = 400;
    response.body = "key rejected";
    httpClient.setResponse(response);
    result = executor.execute(request(RemoteActionType::Ok));
    assert(!result.success);
    assert(result.failureKind == RemoteActionFailureKind::BackendRejected);
    assert(result.backendStatusCode == 400);

    response.statusCode = 503;
    httpClient.setResponse(response);
    result = executor.execute(request(RemoteActionType::Ok));
    assert(!result.success);
    assert(result.failureKind == RemoteActionFailureKind::BackendFailure);
    assert(result.backendStatusCode == 503);

    response.statusCode = 0;
    httpClient.setResponse(response);
    result = executor.execute(request(RemoteActionType::Ok));
    assert(!result.success);
    assert(result.failureKind == RemoteActionFailureKind::Transport);

    const std::size_t before = httpClient.requestCount();
    result = executor.execute(request(RemoteActionType::Invalid));
    assert(!result.success);
    assert(result.failureKind == RemoteActionFailureKind::Validation);
    assert(httpClient.requestCount() == before);

    ThrowingHttpClient throwingHttpClient;
    RestfulApiRemoteActionExecutor throwingExecutor("default", "", throwingHttpClient);
    result = throwingExecutor.execute(request(RemoteActionType::Ok));
    assert(!result.success);
    assert(result.failureKind == RemoteActionFailureKind::Transport);
    return 0;
}
