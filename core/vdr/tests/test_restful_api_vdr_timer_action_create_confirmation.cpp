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

void distinguishesWeeklySchedulesDuringCreate()
{
    const std::string weekly = R"({"id":"74","channel":"C-1-1079-10351","weekdays":"M-W-F--","start":2200,"stop":2300,"file":"Testfilm","flags":4,"priority":0,"lifetime":0,"aux":"eventId=13483"})";
    const std::string differentWeekly = R"({"id":"75","channel":"C-1-1079-10351","weekdays":"-T-T---","start":2200,"stop":2300,"file":"Testfilm"})";
    const std::string oneShot = matchingTimerResponse();
    const std::string existing = oneShot.substr(0, oneShot.size() - 2) + "," + differentWeekly + "]}";
    const std::string created = existing.substr(0, existing.size() - 2) + "," + weekly + "]}";

    auto request = createRequest();
    request.day.clear();
    request.weekdays = "M-W-F--";
    request.active = false;
    request.vps = true;
    request.priority = 0;
    request.lifetime = 0;

    SequenceHttpClient httpClient({
        makeResponse(200, existing),
        makeResponse(201, "timer created"),
        makeResponse(200, created)
    });
    RestfulApiVdrTimerActionExecutor executor("default", "/api", httpClient);
    const auto result = executor.execute(VdrTimerActionType::Create, request);
    assert(result.success);
    assert(result.timerId == "74");
    assert(httpClient.requests().size() == 3);
    const auto& post = httpClient.requests().at(1);
    assert(post.method == "POST");
    assert(post.body.find("weekdays=M-W-F--") != std::string::npos);
    assert(post.body.find("flags=4") != std::string::npos);
    assert(post.body.find("priority=0") != std::string::npos);
    assert(post.body.find("lifetime=0") != std::string::npos);

    SequenceHttpClient absent({
        makeResponse(200, existing),
        makeResponse(201, "timer created"),
        makeResponse(200, existing)
    });
    RestfulApiVdrTimerActionExecutor absentExecutor("default", "/api", absent);
    assert(!absentExecutor.execute(VdrTimerActionType::Create, request).success);
    assert(absent.requests().size() == 3);

    SequenceHttpClient duplicate({makeResponse(200, created)});
    RestfulApiVdrTimerActionExecutor duplicateExecutor("default", "/api", duplicate);
    const auto duplicateResult = duplicateExecutor.execute(VdrTimerActionType::Create, request);
    assert(!duplicateResult.success);
    assert(duplicateResult.timerId == "74");
    assert(duplicate.requests().size() == 1);
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
    distinguishesWeeklySchedulesDuringCreate();
    rejectsPreflightHttpErrorWithoutPost();
    rejectsMissingReadbackEntry();
    rejectsReadbackHttpError();
    return 0;
}
