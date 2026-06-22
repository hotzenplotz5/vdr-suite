#include "BasicHttpClient.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RestfulApiSearchTimerCommandExecutor.h"
#include "SearchTimerCreateRequest.h"
#include "SearchTimerDeleteRequest.h"
#include "SearchTimerUpdateRequest.h"

#include <cstdlib>
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

HttpResponse getSearchTimers(
    BasicHttpClient& client)
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/searchtimers.json";
    request.headers["Accept"] = "application/json";
    return client.execute(request);
}

SearchTimerCreateRequest makeCreateRequest()
{
    SearchTimerCreateRequest request;
    request.backendId = "real-vdr";
    request.name = "[VDR-SUITE-TEST] SearchTimer Smoke Create";
    request.query = "[VDR-SUITE-TEST] SearchTimer Smoke Create";
    request.active = true;
    request.directory = "VDR-SUITE-TEST";
    request.priority = 50;
    request.lifetime = 99;
    request.marginStartMinutes = 5;
    request.marginStopMinutes = 10;
    request.useVps = true;
    request.useChannel = 0;
    request.useTime = true;
    request.startTime = 2015;
    request.stopTime = 2230;
    request.useDuration = true;
    request.durationMinMinutes = 45;
    request.durationMaxMinutes = 120;
    request.useDayOfWeek = true;
    request.dayOfWeek = 62;
    request.avoidRepeats = true;
    request.allowedRepeats = 2;
    request.repeatsWithinDays = 14;
    request.compareTitle = true;
    request.compareSubtitle = true;
    request.compareSummary = true;
    request.compareCategories = true;
    request.compareTime = true;
    return request;
}

SearchTimerUpdateRequest makeUpdateRequest(
    const std::string& nativeId)
{
    SearchTimerUpdateRequest request;
    request.backendId = "real-vdr";
    request.backendNativeId = nativeId;
    request.name = "[VDR-SUITE-TEST] SearchTimer Smoke Update";
    request.query = "[VDR-SUITE-TEST] SearchTimer Smoke Update";
    request.active = true;
    request.directory = "VDR-SUITE-TEST-UPDATED";
    request.priority = 60;
    request.lifetime = 88;
    request.marginStartMinutes = 7;
    request.marginStopMinutes = 12;
    request.useVps = true;
    request.useChannel = 0;
    request.useTime = true;
    request.startTime = 2100;
    request.stopTime = 2300;
    request.useDuration = true;
    request.durationMinMinutes = 30;
    request.durationMaxMinutes = 90;
    request.useDayOfWeek = true;
    request.dayOfWeek = 31;
    request.avoidRepeats = true;
    request.allowedRepeats = 3;
    request.repeatsWithinDays = 21;
    request.compareTitle = true;
    request.compareSubtitle = false;
    request.compareSummary = true;
    request.compareCategories = false;
    request.compareTime = true;
    return request;
}

void printHelp()
{
    std::cout
        << "SearchTimer real VDR smoke helper\n\n"
        << "This tool talks to a real VDR RESTfulAPI instance.\n"
        << "It creates, reads back, updates and deletes a temporary SearchTimer.\n\n"
        << "Environment:\n"
        << "  VDR_SUITE_RESTFULAPI_HOST   default: 127.0.0.1\n"
        << "  VDR_SUITE_RESTFULAPI_PORT   default: 8002\n\n"
        << "Usage:\n"
        << "  /tmp/vdr_suite_searchtimer_real_smoke --help\n"
        << "  /tmp/vdr_suite_searchtimer_real_smoke --run\n\n"
        << "Safety:\n"
        << "  Without --run no SearchTimer is created.\n";
}

int runSmoke()
{
    const std::string host =
        readEnv("VDR_SUITE_RESTFULAPI_HOST", "127.0.0.1");
    const int port =
        parsePort(std::getenv("VDR_SUITE_RESTFULAPI_PORT"));

    std::vector<CheckResult> checks;

    BasicHttpClient client(host, port);
    RestfulApiSearchTimerCommandExecutor executor(client);

    std::string createdNativeId;

    try {
        const HttpResponse before =
            getSearchTimers(client);

        addCheck(
            checks,
            "CONNECT /searchtimers.json",
            before.statusCode == 200,
            "status=" + std::to_string(before.statusCode));

        const SearchTimerCreateRequest createRequest =
            makeCreateRequest();

        const SearchTimerCreateResult createResult =
            executor.create(createRequest);

        addCheck(
            checks,
            "CREATE",
            createResult.success,
            createResult.message);

        if (createResult.success) {
            createdNativeId =
                createResult.searchTimer.id().nativeId();
        }

        const HttpResponse afterCreate =
            getSearchTimers(client);

        addCheck(
            checks,
            "READBACK after create",
            afterCreate.statusCode == 200 &&
                contains(afterCreate.body, createRequest.query),
            "status=" + std::to_string(afterCreate.statusCode));

        addCheck(checks, "FIELD directory create", contains(afterCreate.body, "VDR-SUITE-TEST"));
        addCheck(checks, "FIELD priority create", contains(afterCreate.body, "\"priority\":50") || contains(afterCreate.body, "\"priority\": 50"));
        addCheck(checks, "FIELD lifetime create", contains(afterCreate.body, "\"lifetime\":99") || contains(afterCreate.body, "\"lifetime\": 99"));
        addCheck(checks, "FIELD margin_start create", contains(afterCreate.body, "\"margin_start\":5") || contains(afterCreate.body, "\"margin_start\": 5"));
        addCheck(checks, "FIELD margin_stop create", contains(afterCreate.body, "\"margin_stop\":10") || contains(afterCreate.body, "\"margin_stop\": 10"));
        addCheck(checks, "FIELD use_vps create", contains(afterCreate.body, "\"use_vps\":1") || contains(afterCreate.body, "\"use_vps\": 1"));
        addCheck(checks, "FIELD use_time create", contains(afterCreate.body, "\"use_time\":1") || contains(afterCreate.body, "\"use_time\": 1"));
        addCheck(checks, "FIELD use_duration create", contains(afterCreate.body, "\"use_duration\":1") || contains(afterCreate.body, "\"use_duration\": 1"));
        addCheck(checks, "FIELD avoid_repeats create", contains(afterCreate.body, "\"avoid_repeats\":1") || contains(afterCreate.body, "\"avoid_repeats\": 1"));

        if (!createdNativeId.empty()) {
            const SearchTimerUpdateRequest updateRequest =
                makeUpdateRequest(createdNativeId);

            const SearchTimerUpdateResult updateResult =
                executor.update(updateRequest);

            addCheck(
                checks,
                "UPDATE",
                updateResult.success,
                updateResult.message);

            const HttpResponse afterUpdate =
                getSearchTimers(client);

            addCheck(
                checks,
                "READBACK after update",
                afterUpdate.statusCode == 200 &&
                    contains(afterUpdate.body, updateRequest.query),
                "status=" + std::to_string(afterUpdate.statusCode));

            addCheck(checks, "FIELD directory update", contains(afterUpdate.body, "VDR-SUITE-TEST-UPDATED"));
            addCheck(checks, "FIELD priority update", contains(afterUpdate.body, "\"priority\":60") || contains(afterUpdate.body, "\"priority\": 60"));
            addCheck(checks, "FIELD allowed_repeats update", contains(afterUpdate.body, "\"allowed_repeats\":3") || contains(afterUpdate.body, "\"allowed_repeats\": 3"));

            SearchTimerDeleteRequest deleteRequest;
            deleteRequest.backendId = "real-vdr";
            deleteRequest.backendNativeId = createdNativeId;

            const SearchTimerDeleteResult deleteResult =
                executor.remove(deleteRequest);

            addCheck(
                checks,
                "DELETE cleanup",
                deleteResult.success,
                deleteResult.message);

            const HttpResponse afterDelete =
                getSearchTimers(client);

            addCheck(
                checks,
                "READBACK after delete",
                afterDelete.statusCode == 200 &&
                    !contains(afterDelete.body, updateRequest.query),
                "status=" + std::to_string(afterDelete.statusCode));
        }
    }
    catch (const std::exception& error) {
        addCheck(checks, "EXCEPTION", false, error.what());

        if (!createdNativeId.empty()) {
            try {
                SearchTimerDeleteRequest cleanup;
                cleanup.backendId = "real-vdr";
                cleanup.backendNativeId = createdNativeId;
                executor.remove(cleanup);
            }
            catch (...) {
            }
        }
    }

    std::cout << "SearchTimer Real VDR Smoke Test\n";
    std::cout << "host=" << host << "\n";
    std::cout << "port=" << port << "\n\n";

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
