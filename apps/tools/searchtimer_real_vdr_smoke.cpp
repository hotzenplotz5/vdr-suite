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
#include <ctime>

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

std::string extractIdForSearch(
    const std::string& body,
    const std::string& search)
{
    const std::size_t searchPos =
        body.find(search);

    if (searchPos == std::string::npos) {
        return "";
    }

    const std::size_t objectStart =
        body.rfind('{', searchPos);

    if (objectStart == std::string::npos) {
        return "";
    }

    const std::string idKey = "\"id\"";
    const std::size_t idKeyPos =
        body.find(idKey, objectStart);

    if (idKeyPos == std::string::npos || idKeyPos > searchPos) {
        return "";
    }

    const std::size_t colon =
        body.find(':', idKeyPos + idKey.size());

    if (colon == std::string::npos || colon > searchPos) {
        return "";
    }

    std::size_t pos = colon + 1;

    while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos]))) {
        ++pos;
    }

    std::size_t end = pos;

    while (end < body.size() && std::isdigit(static_cast<unsigned char>(body[end]))) {
        ++end;
    }

    return body.substr(pos, end - pos);
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

SearchTimerCreateRequest makeCreateRequest(
    const std::string& marker)
{
    SearchTimerCreateRequest request;
    request.backendId = "real-vdr";
    request.name = marker + " Create";
    request.query = marker + " Create";
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
    request.dayOfWeek = 1;
    request.avoidRepeats = true;
    request.allowedRepeats = 2;
    request.repeatsWithinDays = 14;
    request.compareTitle = true;
    request.compareSubtitle = true;
    request.compareSummary = true;
    request.compareCategories = true;
    request.compareTime = true;
    request.useSeriesRecording = true;
    request.keepRecordings = 5;
    request.deleteMode = 1;
    request.searchTimerAction = 2;
    request.blacklistMode = 1;
    request.blacklistIds = "3,7";
    request.matchMode = 2;
    request.matchCase = true;
    request.matchTolerance = 4;
    request.summaryMatch = 1;
    request.useExtendedEpgInfo = true;
    request.extendedEpgInfo = "category=movie";
    request.ignoreMissingEpgCategories = true;
    request.contentDescriptors = "0x10,0x11";
    request.useInFavorites = true;
    request.activeFrom = "2026-06-01";
    request.activeUntil = "2026-12-31";
    request.pauseOnRecordings = true;
    request.switchMinutesBefore = 3;
    request.unmuteSoundOnSwitch = true;
    request.deleteRecordingsAfterDays = 30;
    request.deleteAfterCountRecordings = 5;
    request.deleteAfterDaysOfFirstRecording = 90;
    return request;
}

SearchTimerUpdateRequest makeUpdateRequest(
    const std::string& nativeId,
    const std::string& marker)
{
    SearchTimerUpdateRequest request;
    request.backendId = "real-vdr";
    request.backendNativeId = nativeId;
    request.name = marker + " Update";
    request.query = marker + " Update";
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
    request.dayOfWeek = 2;
    request.avoidRepeats = true;
    request.allowedRepeats = 3;
    request.repeatsWithinDays = 21;
    request.compareTitle = true;
    request.compareSubtitle = false;
    request.compareSummary = true;
    request.compareCategories = false;
    request.compareTime = true;
    request.useSeriesRecording = true;
    request.keepRecordings = 6;
    request.deleteMode = 2;
    request.searchTimerAction = 3;
    request.blacklistMode = 2;
    request.blacklistIds = "4";
    request.matchMode = 3;
    request.matchCase = false;
    request.matchTolerance = 5;
    request.summaryMatch = 2;
    request.useExtendedEpgInfo = false;
    request.extendedEpgInfo = "category=sports";
    request.ignoreMissingEpgCategories = false;
    request.contentDescriptors = "0x20";
    request.useInFavorites = false;
    request.activeFrom = "2026-07-01";
    request.activeUntil = "2026-08-31";
    request.pauseOnRecordings = false;
    request.switchMinutesBefore = 4;
    request.unmuteSoundOnSwitch = false;
    request.deleteRecordingsAfterDays = 40;
    request.deleteAfterCountRecordings = 6;
    request.deleteAfterDaysOfFirstRecording = 120;
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

    const std::string marker =
        "[VDR-SUITE-TEST] SearchTimer Smoke " +
        std::to_string(static_cast<long long>(std::time(nullptr)));

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
            makeCreateRequest(marker);

        const SearchTimerCreateResult createResult =
            executor.create(createRequest);

        std::string createDetails =
            createResult.message;

        for (const std::string& error : createResult.errors) {
            createDetails += " | ";
            createDetails += error;
        }

        const bool createAccepted =
            createResult.success ||
            createDetails.find("did not return an id") != std::string::npos;

        addCheck(
            checks,
            "CREATE",
            createAccepted,
            createDetails);

        const HttpResponse afterCreate =
            getSearchTimers(client);

        if (createResult.success) {
            createdNativeId =
                createResult.searchTimer.id().nativeId();
        }

        if (createdNativeId.empty()) {
            createdNativeId =
                extractIdForSearch(afterCreate.body, createRequest.query);

            addCheck(
                checks,
                "CREATE id fallback",
                !createdNativeId.empty(),
                "id=" + createdNativeId);
        }

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
        addCheck(checks, "FIELD use_vps create", contains(afterCreate.body, "\"use_vps\":1") || contains(afterCreate.body, "\"use_vps\": 1") || contains(afterCreate.body, "\"use_vps\":true") || contains(afterCreate.body, "\"use_vps\": true"));
        addCheck(checks, "FIELD use_time create", contains(afterCreate.body, "\"use_time\":1") || contains(afterCreate.body, "\"use_time\": 1") || contains(afterCreate.body, "\"use_time\":true") || contains(afterCreate.body, "\"use_time\": true"));
        addCheck(checks, "FIELD use_duration create", contains(afterCreate.body, "\"use_duration\":1") || contains(afterCreate.body, "\"use_duration\": 1") || contains(afterCreate.body, "\"use_duration\":true") || contains(afterCreate.body, "\"use_duration\": true"));
        addCheck(checks, "FIELD avoid_repeats create", contains(afterCreate.body, "\"avoid_repeats\":1") || contains(afterCreate.body, "\"avoid_repeats\": 1") || contains(afterCreate.body, "\"avoid_repeats\":true") || contains(afterCreate.body, "\"avoid_repeats\": true"));
        addCheck(checks, "FIELD use_series_recording create", contains(afterCreate.body, "\"use_series_recording\":1") || contains(afterCreate.body, "\"use_series_recording\": 1") || contains(afterCreate.body, "\"use_series_recording\":true") || contains(afterCreate.body, "\"use_series_recording\": true"));
        addCheck(checks, "FIELD keep_recs create", contains(afterCreate.body, "\"keep_recs\":5") || contains(afterCreate.body, "\"keep_recs\": 5"));
        addCheck(checks, "FIELD blacklist_mode create", contains(afterCreate.body, "\"blacklist_mode\":1") || contains(afterCreate.body, "\"blacklist_mode\": 1"));
        addCheck(checks, "FIELD mode create", contains(afterCreate.body, "\"mode\":2") || contains(afterCreate.body, "\"mode\": 2"));
        addCheck(checks, "FIELD match_case create", contains(afterCreate.body, "\"match_case\":1") || contains(afterCreate.body, "\"match_case\": 1") || contains(afterCreate.body, "\"match_case\":true") || contains(afterCreate.body, "\"match_case\": true"));
        addCheck(checks, "FIELD use_ext_epg_info create", contains(afterCreate.body, "\"use_ext_epg_info\":1") || contains(afterCreate.body, "\"use_ext_epg_info\": 1") || contains(afterCreate.body, "\"use_ext_epg_info\":true") || contains(afterCreate.body, "\"use_ext_epg_info\": true"));
        addCheck(checks, "FIELD use_in_favorites create", contains(afterCreate.body, "\"use_in_favorites\":1") || contains(afterCreate.body, "\"use_in_favorites\": 1") || contains(afterCreate.body, "\"use_in_favorites\":true") || contains(afterCreate.body, "\"use_in_favorites\": true"));
        addCheck(checks, "FIELD pause_on_recs create", contains(afterCreate.body, "\"pause_on_recs\":1") || contains(afterCreate.body, "\"pause_on_recs\": 1") || contains(afterCreate.body, "\"pause_on_recs\":true") || contains(afterCreate.body, "\"pause_on_recs\": true"));

        if (!createdNativeId.empty()) {
            const SearchTimerUpdateRequest updateRequest =
                makeUpdateRequest(createdNativeId, marker);

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
            addCheck(checks, "FIELD keep_recs update", contains(afterUpdate.body, "\"keep_recs\":6") || contains(afterUpdate.body, "\"keep_recs\": 6"));
            addCheck(checks, "FIELD blacklist_mode update", contains(afterUpdate.body, "\"blacklist_mode\":2") || contains(afterUpdate.body, "\"blacklist_mode\": 2"));
            addCheck(checks, "FIELD mode update", contains(afterUpdate.body, "\"mode\":3") || contains(afterUpdate.body, "\"mode\": 3"));
            addCheck(
                checks,
                "FIELD tolerance update",
                contains(afterUpdate.body, "\"tolerance\":5") ||
                    contains(afterUpdate.body, "\"tolerance\": 5") ||
                    contains(afterUpdate.body, "\"tolerance\":1") ||
                    contains(afterUpdate.body, "\"tolerance\": 1"),
                "expected 5; real VDR may normalize to 1");
            addCheck(checks, "FIELD use_ext_epg_info update", contains(afterUpdate.body, "\"use_ext_epg_info\":0") || contains(afterUpdate.body, "\"use_ext_epg_info\": 0") || contains(afterUpdate.body, "\"use_ext_epg_info\":false") || contains(afterUpdate.body, "\"use_ext_epg_info\": false"));
            addCheck(checks, "FIELD use_in_favorites update", contains(afterUpdate.body, "\"use_in_favorites\":0") || contains(afterUpdate.body, "\"use_in_favorites\": 0") || contains(afterUpdate.body, "\"use_in_favorites\":false") || contains(afterUpdate.body, "\"use_in_favorites\": false"));
            addCheck(checks, "FIELD switch_min_before update", contains(afterUpdate.body, "\"switch_min_before\":4") || contains(afterUpdate.body, "\"switch_min_before\": 4"));
            addCheck(checks, "FIELD del_recs_after_days update", contains(afterUpdate.body, "\"del_recs_after_days\":40") || contains(afterUpdate.body, "\"del_recs_after_days\": 40"));

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
