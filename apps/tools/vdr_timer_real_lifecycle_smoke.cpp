#include "BasicHttpClient.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RestfulApiVdrTimerActionExecutor.h"
#include "VdrTimerOperationRequest.h"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
struct CheckResult
{
    std::string name;
    bool passed = false;
    std::string details;
};

std::string readEnv(
    const char* name,
    const std::string& fallback)
{
    const char* value = std::getenv(name);

    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }

    return value;
}

int parsePort(
    const char* value)
{
    if (value == nullptr) {
        return 8002;
    }

    return std::stoi(value);
}

void addCheck(
    std::vector<CheckResult>& checks,
    const std::string& name,
    bool passed,
    const std::string& details = "")
{
    checks.push_back(CheckResult{name, passed, details});
}

bool contains(
    const std::string& text,
    const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

std::string resultDetails(
    const VdrTimerActionResult& result)
{
    std::string details =
        result.message;

    for (const std::string& error : result.errors) {
        details += " | ";
        details += error;
    }

    return details;
}

HttpResponse getTimers(
    BasicHttpClient& client)
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/timers.json";
    request.headers["Accept"] = "application/json";
    return client.execute(request);
}

std::string extractIdForTitle(
    const std::string& body,
    const std::string& title)
{
    const std::size_t titlePos =
        body.find(title);

    if (titlePos == std::string::npos) {
        return "";
    }

    const std::size_t objectStart =
        body.rfind('{', titlePos);

    if (objectStart == std::string::npos) {
        return "";
    }

    const std::string idKey = "\"id\"";
    const std::size_t idKeyPos =
        body.find(idKey, objectStart);

    if (idKeyPos == std::string::npos || idKeyPos > titlePos) {
        return "";
    }

    const std::size_t colon =
        body.find(':', idKeyPos + idKey.size());

    if (colon == std::string::npos || colon > titlePos) {
        return "";
    }

    std::size_t pos = colon + 1;

    while (pos < body.size() &&
           std::isspace(static_cast<unsigned char>(body[pos]))) {
        ++pos;
    }

    if (pos < body.size() && body[pos] == '"') {
        ++pos;

        const std::size_t end =
            body.find('"', pos);

        if (end == std::string::npos) {
            return "";
        }

        return body.substr(pos, end - pos);
    }

    std::size_t end = pos;

    while (end < body.size() &&
           std::isdigit(static_cast<unsigned char>(body[end]))) {
        ++end;
    }

    return body.substr(pos, end - pos);
}

std::string tomorrowDay()
{
    std::time_t now =
        std::time(nullptr) + 24 * 60 * 60;

    std::tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    char buffer[16] = {0};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);
    return std::string(buffer);
}

VdrTimerOperationRequest makeCreateRequest(
    const std::string& marker)
{
    VdrTimerOperationRequest request;
    request.backendId = "real-vdr";
    request.channelId =
        readEnv("VDR_SUITE_TIMER_CHANNEL", "");
    request.title =
        marker + "_CREATE";
    request.directory =
        "VDR-SUITE-TEST";
    request.day =
        readEnv("VDR_SUITE_TIMER_DAY", tomorrowDay());
    request.weekdays = "-------";
    request.start = 2350;
    request.stop = 2355;
    request.priority = 50;
    request.lifetime = 99;
    request.active = false;
    request.aux =
        "VDR-SUITE-REAL-TIMER-LIFECYCLE";
    return request;
}

VdrTimerOperationRequest makeUpdateRequest(
    const std::string& timerId,
    const std::string& marker)
{
    VdrTimerOperationRequest request;
    request.backendId = "real-vdr";
    request.timerId = timerId;
    request.channelId =
        readEnv("VDR_SUITE_TIMER_CHANNEL", "");
    request.title =
        marker + "_UPDATE";
    request.directory =
        "VDR-SUITE-TEST";
    request.day =
        readEnv("VDR_SUITE_TIMER_DAY", tomorrowDay());
    request.weekdays = "-------";
    request.start = 2345;
    request.stop = 2355;
    request.priority = 60;
    request.lifetime = 88;
    request.active = false;
    request.aux =
        "VDR-SUITE-REAL-TIMER-LIFECYCLE-UPDATED";
    return request;
}

void printHelp()
{
    std::cout
        << "VDR Timer real VDR lifecycle smoke helper\n\n"
        << "This tool talks to a real VDR RESTfulAPI instance.\n"
        << "It creates, reads back, updates and deletes a temporary inactive Timer.\n\n"
        << "Environment:\n"
        << "  VDR_SUITE_RESTFULAPI_HOST   default: 127.0.0.1\n"
        << "  VDR_SUITE_RESTFULAPI_PORT   default: 8002\n"
        << "  VDR_SUITE_TIMER_CHANNEL     required: real VDR channel id, e.g. S19.2E-...\n"
        << "  VDR_SUITE_TIMER_DAY         default: tomorrow as YYYY-MM-DD\n\n"
        << "Usage:\n"
        << "  /tmp/vdr_suite_timer_lifecycle_smoke --help\n"
        << "  /tmp/vdr_suite_timer_lifecycle_smoke --run\n\n"
        << "Safety:\n"
        << "  Without --run no Timer is created.\n"
        << "  The created Timer is inactive and carries a VDR-SUITE marker.\n";
}

int runSmoke()
{
    const std::string host =
        readEnv("VDR_SUITE_RESTFULAPI_HOST", "127.0.0.1");
    const int port =
        parsePort(std::getenv("VDR_SUITE_RESTFULAPI_PORT"));

    std::vector<CheckResult> checks;

    BasicHttpClient client(host, port);
    RestfulApiVdrTimerActionExecutor executor(
        "real-vdr",
        "",
        client);

    const std::string configuredChannel =
        readEnv("VDR_SUITE_TIMER_CHANNEL", "");

    if (configuredChannel.empty()) {
        std::cerr
            << "VDR_SUITE_TIMER_CHANNEL must be set to a real VDR channel id, "
            << "not a channel number." << std::endl;
        return 1;
    }

    const std::string marker =
        "VDR-SUITE-TIMER-TEST-" +
        std::to_string(static_cast<long long>(std::time(nullptr)));

    std::string createdTimerId;

    try {
        const HttpResponse before =
            getTimers(client);

        addCheck(
            checks,
            "CONNECT /timers.json",
            before.statusCode == 200,
            "status=" + std::to_string(before.statusCode));

        const VdrTimerOperationRequest createRequest =
            makeCreateRequest(marker);

        const VdrTimerActionResult createResult =
            executor.execute(
                VdrTimerActionType::Create,
                createRequest);

        addCheck(
            checks,
            "CREATE inactive timer",
            createResult.success,
            resultDetails(createResult));

        const HttpResponse afterCreate =
            getTimers(client);

        createdTimerId =
            extractIdForTitle(afterCreate.body, createRequest.title);

        addCheck(
            checks,
            "CREATE id fallback",
            !createdTimerId.empty(),
            "id=" + createdTimerId);

        addCheck(
            checks,
            "READBACK after create",
            afterCreate.statusCode == 200 &&
                contains(afterCreate.body, createRequest.title),
            "status=" + std::to_string(afterCreate.statusCode));

        addCheck(
            checks,
            "FIELD directory create",
            contains(afterCreate.body, "VDR-SUITE-TEST"));

        addCheck(
            checks,
            "FIELD priority create",
            contains(afterCreate.body, "\"priority\":50") ||
                contains(afterCreate.body, "\"priority\": 50"));

        addCheck(
            checks,
            "FIELD lifetime create",
            contains(afterCreate.body, "\"lifetime\":99") ||
                contains(afterCreate.body, "\"lifetime\": 99"));

        if (!createdTimerId.empty()) {
            const VdrTimerOperationRequest updateRequest =
                makeUpdateRequest(createdTimerId, marker);

            const VdrTimerActionResult updateResult =
                executor.execute(
                    VdrTimerActionType::Update,
                    updateRequest);

            addCheck(
                checks,
                "UPDATE inactive timer",
                updateResult.success,
                resultDetails(updateResult));

            const HttpResponse afterUpdate =
                getTimers(client);

            addCheck(
                checks,
                "READBACK after update",
                afterUpdate.statusCode == 200 &&
                    contains(afterUpdate.body, updateRequest.title),
                "status=" + std::to_string(afterUpdate.statusCode));

            addCheck(
                checks,
                "FIELD priority update",
                contains(afterUpdate.body, "\"priority\":60") ||
                    contains(afterUpdate.body, "\"priority\": 60"));

            addCheck(
                checks,
                "FIELD lifetime update",
                contains(afterUpdate.body, "\"lifetime\":88") ||
                    contains(afterUpdate.body, "\"lifetime\": 88"));

            const std::string updatedTimerId =
                extractIdForTitle(afterUpdate.body, updateRequest.title);

            addCheck(
                checks,
                "UPDATE id refresh",
                !updatedTimerId.empty(),
                "id=" + updatedTimerId);

            VdrTimerOperationRequest deleteRequest;
            deleteRequest.backendId = "real-vdr";
            deleteRequest.timerId =
                updatedTimerId.empty() ? createdTimerId : updatedTimerId;

            const VdrTimerActionResult deleteResult =
                executor.execute(
                    VdrTimerActionType::Delete,
                    deleteRequest);

            addCheck(
                checks,
                "DELETE cleanup",
                deleteResult.success,
                resultDetails(deleteResult));

            const HttpResponse afterDelete =
                getTimers(client);

            addCheck(
                checks,
                "READBACK after delete",
                afterDelete.statusCode == 200 &&
                    !contains(afterDelete.body, marker),
                "status=" + std::to_string(afterDelete.statusCode));
        }
    }
    catch (const std::exception& error) {
        addCheck(checks, "EXCEPTION", false, error.what());

        if (!createdTimerId.empty()) {
            try {
                VdrTimerOperationRequest cleanup;
                cleanup.backendId = "real-vdr";
                cleanup.timerId = createdTimerId;

                executor.execute(
                    VdrTimerActionType::Delete,
                    cleanup);
            }
            catch (...) {
            }
        }
    }

    std::cout << "VDR Timer Real VDR Lifecycle Smoke Test\n";
    std::cout << "host=" << host << "\n";
    std::cout << "port=" << port << "\n";
    std::cout << "channel=" << readEnv("VDR_SUITE_TIMER_CHANNEL", "") << "\n\n";

    bool allPassed = true;

    for (const CheckResult& check : checks) {
        allPassed = allPassed && check.passed;

        std::cout
            << (check.passed ? "PASS " : "FAIL ")
            << check.name;

        if (!check.details.empty()) {
            std::cout << " - " << check.details;
        }

        std::cout << "\n";
    }

    std::cout << "\nResult: "
              << (allPassed ? "SUCCESS" : "FAILURE")
              << std::endl;

    return allPassed ? 0 : 2;
}
}

int main(int argc, char** argv)
{
    if (argc < 2 || std::string(argv[1]) == "--help") {
        printHelp();
        return 0;
    }

    if (std::string(argv[1]) != "--run") {
        std::cerr << "Refusing to modify real VDR without --run." << std::endl;
        printHelp();
        return 1;
    }

    return runSmoke();
}
