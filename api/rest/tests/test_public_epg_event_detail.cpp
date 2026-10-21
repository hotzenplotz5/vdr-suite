#include "PublicApiRuntime.h"
#include <cassert>
#include <string>
#include <vector>

int main()
{
    auto& rt = PublicApiRuntime::instance();
    const std::string path =
        "/api/v1/epg/event?backendId=home&channelId=S19.2E-1-100&eventId=81234";
    auto read = [&rt](const std::string& target,
        const std::string& actor = "actor",
        const std::vector<std::string>& scope = {"home"},
        const std::string& ifNoneMatch = "") {
        ApiResponse response;
        assert(rt.tryHandleGet(target, actor, "request-id", "corr-id",
            response, ifNoneMatch, "", scope));
        return response;
    };
    rt.resetEpgEventDetailLookup();
    assert(read(path).statusCode == 503);
    assert(read(path, "").statusCode == 401);
    assert(read(path,"actor",{"foreign"}).statusCode == 400);
    for (const std::string& invalid : {
        "/api/v1/epg/event",
        "/api/v1/epg/event?backendId=home&channelId=a&eventId=",
        "/api/v1/epg/event?backendId=home&channelId=a&eventId=1&eventId=2",
        "/api/v1/epg/event?backendId=home&channelId=../x&eventId=1",
        "/api/v1/epg/event?backendId=home&channelId=a&eventId=1&legacy=1"})
        assert(read(invalid).statusCode == 400);
    int calls = 0;
    rt.registerEpgEventDetailLookup([&](const PublicEpgEventDetailRequest& q) {
        ++calls;
        PublicEpgEventDetailResult r;
        r.status = PublicEpgNowNextStatus::ok;
        r.backendId = q.backendId; r.channelId = q.channelId;
        r.eventId = q.eventId; r.title = "Programme";
        r.description = "Detail"; r.startTime = "1792222800";
        r.endTime = "1792226400"; r.durationSeconds = 3600;
        r.resourceRevision = "epg1:sha256:" + std::string(64, 'a');
        return r;
    });
    const auto ok = read(path);
    assert(ok.statusCode == 200);
    assert(ok.body.find("Programme") != std::string::npos);
    assert(ok.body.find("Detail") != std::string::npos);
    assert(ok.body.find("backendGeneration") == std::string::npos);
    assert(ok.headers.count("ETag") == 1U);
    assert(ok.headers.at("Cache-Control") == "no-store");
    const auto cached = read(path, "actor", {"home"}, ok.headers.at("ETag"));
    assert(cached.statusCode == 304 && cached.body.empty());
    assert(read(path, "actor", {"home"}, "W/\"bad").statusCode == 400);
    assert(calls == 3);
    rt.registerEpgEventDetailLookup([](const PublicEpgEventDetailRequest& q) {
        PublicEpgEventDetailResult r;
        r.status = PublicEpgNowNextStatus::ok;
        r.backendId = "wrong"; r.channelId = q.channelId;
        r.eventId = q.eventId; r.title = "Leak";
        r.startTime = "1792222800"; r.endTime = "1792226400";
        r.resourceRevision = "other";
        return r;
    });
    assert(read(path).statusCode == 503);
    rt.registerEpgEventDetailLookup([](const PublicEpgEventDetailRequest&) {
        PublicEpgEventDetailResult r;
        r.status = PublicEpgNowNextStatus::notFound;
        return r;
    });
    assert(read(path).statusCode == 404);
    rt.resetEpgEventDetailLookup();
    return 0;
}
