#include "RestfulApiVdrTimerActionExecutor.h"
#include "IHttpClient.h"

#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace
{
class SequenceHttpClient final : public IHttpClient
{
public:
    explicit SequenceHttpClient(std::vector<HttpResponse> responses)
        : responses_(std::move(responses))
    {
    }

    HttpResponse execute(const HttpRequest& request) const override
    {
        requests_.push_back(request);
        const std::size_t index = requests_.size() - 1;
        return index < responses_.size()
            ? responses_.at(index)
            : HttpResponse{};
    }

    const std::vector<HttpRequest>& requests() const
    {
        return requests_;
    }

private:
    std::vector<HttpResponse> responses_;
    mutable std::vector<HttpRequest> requests_;
};

HttpResponse makeResponse(
    int statusCode,
    const std::string& body)
{
    HttpResponse result;
    result.statusCode = statusCode;
    result.body = body;
    return result;
}

VdrTimerOperationRequest createRequest()
{
    VdrTimerOperationRequest request;
    request.backendId = "default";
    request.channelId = "C-1-1079-10351";
    request.title = "Testfilm";
    request.day = "2026-07-21";
    request.weekdays = "-------";
    request.start = 2200;
    request.stop = 2300;
    request.priority = 50;
    request.lifetime = 99;
    request.active = true;
    request.aux = "eventId=13483";
    return request;
}

std::string emptyTimerResponse()
{
    return "{\"timers\":[]}";
}

std::string matchingTimerResponse()
{
    return
        "{\"timers\":[{"
        "\"id\":\"73\","
        "\"channel\":\"C-1-1079-10351\","
        "\"day\":\"2026-07-21\","
        "\"start\":2200,"
        "\"stop\":2300,"
        "\"file\":\"Testfilm\""
        "}]}";
}

void confirmsNewTimerByPreflightAndReadback()
{
    SequenceHttpClient httpClient({
        makeResponse(200, emptyTimerResponse()),
        makeResponse(201, "timer created"),
        makeResponse(200, matchingTimerResponse())
    });

    RestfulApiVdrTimerActionExecutor executor(
        "default",
        "/api",
        httpClient);

    const VdrTimerActionResult result =
        executor.execute(
            VdrTimerActionType::Create,
            createRequest());

    assert(result.success);
    assert(result.timerId == "73");
    assert(result.message ==
           "RESTfulAPI timer creation confirmed by readback");
    assert(httpClient.requests().size() == 3);
    assert(httpClient.requests().at(0).method == "GET");
    assert(httpClient.requests().at(0).url == "/api/timers.json");
    assert(httpClient.requests().at(1).method == "POST");
    assert(httpClient.requests().at(2).method == "GET");
    assert(httpClient.requests().at(2).url == "/api/timers.json");
}

void rejectsPreExistingTimerWithoutPost()
{
    SequenceHttpClient httpClient({
        makeResponse(200, matchingTimerResponse())
    });

    RestfulApiVdrTimerActionExecutor executor(
        "default",
        "/api",
        httpClient);

    const VdrTimerActionResult result =
        executor.execute(
            VdrTimerActionType::Create,
            createRequest());

    assert(!result.success);
    assert(result.timerId == "73");
    assert(result.message == "Timer ist bereits vorhanden.");
    assert(result.errors.size() == 1);
    assert(httpClient.requests().size() == 1);
    assert(httpClient.requests().at(0).method == "GET");
    assert(httpClient.requests().at(0).url == "/api/timers.json");
}

void acceptsNumericTimerIdentityAndStringClockValues()
{
    SequenceHttpClient httpClient({
        makeResponse(200, emptyTimerResponse()),
        makeResponse(200, "Timer created."),
        makeResponse(
            200,
            "{\"timers\":[{"
            "\"number\":74,"
            "\"channel\":\"C-1-1079-10351\","
            "\"day\":\"2026-07-21\","
            "\"start\":\"2200\","
            "\"stop\":\"2300\","
            "\"filename\":\"Testfilm\""
            "}]}")
    });

    RestfulApiVdrTimerActionExecutor executor(
        "default",
        "/api",
        httpClient);

    const VdrTimerActionResult result =
        executor.execute(
            VdrTimerActionType::Create,
            createRequest());

    assert(result.success);
    assert(result.timerId == "74");
}

void rejectsPreflightHttpErrorWithoutPost()
{
    SequenceHttpClient httpClient({
        makeResponse(503, "temporarily unavailable")
    });

    RestfulApiVdrTimerActionExecutor executor(
        "default",
        "/api",
        httpClient);

    const VdrTimerActionResult result =
        executor.execute(
            VdrTimerActionType::Create,
            createRequest());

    assert(!result.success);
    assert(result.timerId.empty());
    assert(result.message ==
           "Timerprüfung vor dem Erstellen ist fehlgeschlagen.");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0).find("HTTP status 503") !=
           std::string::npos);
    assert(httpClient.requests().size() == 1);
}

void rejectsMissingReadbackEntry()
{
    SequenceHttpClient httpClient({
        makeResponse(200, emptyTimerResponse()),
        makeResponse(201, "timer created"),
        makeResponse(
            200,
            "{\"timers\":[{"
            "\"id\":\"74\","
            "\"channel\":\"C-1-1079-10351\","
            "\"day\":\"2026-07-21\","
            "\"start\":2205,"
            "\"stop\":2300,"
            "\"file\":\"Testfilm\""
            "}]}")
    });

    RestfulApiVdrTimerActionExecutor executor(
        "default",
        "/api",
        httpClient);

    const VdrTimerActionResult result =
        executor.execute(
            VdrTimerActionType::Create,
            createRequest());

    assert(!result.success);
    assert(result.timerId.empty());
    assert(result.message ==
           "RESTfulAPI timer creation was not visible in readback");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0).find("channel=C-1-1079-10351") !=
           std::string::npos);
    assert(result.errors.at(0).find("start=2200") !=
           std::string::npos);
}

void rejectsReadbackHttpError()
{
    SequenceHttpClient httpClient({
        makeResponse(200, emptyTimerResponse()),
        makeResponse(201, "timer created"),
        makeResponse(503, "temporarily unavailable")
    });

    RestfulApiVdrTimerActionExecutor executor(
        "default",
        "/api",
        httpClient);

    const VdrTimerActionResult result =
        executor.execute(
            VdrTimerActionType::Create,
            createRequest());

    assert(!result.success);
    assert(result.timerId.empty());
    assert(result.message ==
           "RESTfulAPI timer creation readback failed");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0).find("HTTP status 503") !=
           std::string::npos);
}
}

int main()
{
    confirmsNewTimerByPreflightAndReadback();
    rejectsPreExistingTimerWithoutPost();
    acceptsNumericTimerIdentityAndStringClockValues();
    rejectsPreflightHttpErrorWithoutPost();
    rejectsMissingReadbackEntry();
    rejectsReadbackHttpError();
    return 0;
}
