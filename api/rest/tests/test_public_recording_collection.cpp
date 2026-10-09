#include "PublicApiRuntime.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{
std::string cursorFrom(const std::string& body)
{
    const std::string marker = "\"nextCursor\":\"";
    const auto start = body.find(marker);
    if (start == std::string::npos) return {};
    const auto first = start + marker.size();
    const auto end = body.find('"', first);
    return end == std::string::npos ? std::string() :
        body.substr(first, end - first);
}

ApiResponse get(
    const std::string& path,
    const std::string& actor,
    const std::vector<std::string>& scopes)
{
    ApiResponse response;
    const bool handled = PublicApiRuntime::instance().tryHandleGet(
        path, actor, "recording-test", "recording-correlation",
        response, "", "", scopes);
    assert(handled);
    return response;
}

VdrPublicRecordingSummary record(const std::string& hex, const std::string& date)
{
    VdrPublicRecordingSummary item;
    item.recordingId = "rec_" + hex;
    item.backendId = "home";
    item.title = "Film";
    item.recordedAt = date;
    item.durationKnown = true;
    item.durationSeconds = 120;
    return item;
}
}

int main()
{
    auto& runtime = PublicApiRuntime::instance();
    runtime.resetRecordingCollectionLookup();
    runtime.registerRecordingCollectionLookup([](const std::string& backend) {
        if (backend != "home") return VdrPublicRecordingCollection{{}, false};
        VdrPublicRecordingCollection result;
        result.items = {
            record("00000000000000000000000000000003", "2026-10-07"),
            record("00000000000000000000000000000002", "2026-10-08"),
            record("00000000000000000000000000000001", "2026-10-09")
        };
        return result;
    });
    const auto first = get(
        "/api/v1/recordings?backendId=home&limit=2",
        "actor:test", {"home"});
    assert(first.statusCode == 200);
    assert(first.body.find("\"hasMore\":true") != std::string::npos);
    assert(first.body.find("2026-10-09") < first.body.find("2026-10-08"));
    assert(first.body.find("2026-10-07") == std::string::npos);
    const auto cursor = cursorFrom(first.body);
    assert(cursor.rfind("rc1_", 0U) == 0U);

    const auto second = get(
        "/api/v1/recordings?backendId=home&limit=2&cursor=" + cursor,
        "actor:test", {"home"});
    assert(second.statusCode == 200);
    assert(second.body.find("2026-10-07") != std::string::npos);
    assert(second.body.find("\"hasMore\":false") != std::string::npos);

    assert(get("/api/v1/recordings?backendId=home",
        "", {"home"}).statusCode == 401);
    assert(get("/api/v1/recordings?backendId=other",
        "actor:test", {"home"}).statusCode == 400);
    assert(get("/api/v1/recordings?backendId=home&backendId=home",
        "actor:test", {"home"}).statusCode == 400);
    assert(get("/api/v1/recordings?backendId=home&cursor=evil",
        "actor:test", {"home"}).statusCode == 400);
    assert(get("/api/v1/recordings?backendId=home&limit=999",
        "actor:test", {"home"}).statusCode == 400);
    assert(get("/api/v1/recordings?backendId=home&extra=x",
        "actor:test", {"home"}).statusCode == 400);
    assert(get("/api/v1/recordings?backendId=other&cursor=" + cursor,
        "actor:test", {"other"}).statusCode == 400);
    assert(get("/api/v1/recordings?backendId=home",
        "actor:test", {}).statusCode == 400);
    runtime.resetRecordingCollectionLookup();
    assert(get("/api/v1/recordings?backendId=home",
        "actor:test", {"home"}).statusCode == 503);
    std::cout << "public Recording collection runtime: PASS" << std::endl;
}
