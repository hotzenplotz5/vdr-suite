#include "PublicApiRuntime.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

namespace
{
const std::string Request =
    "/api/v1/epg/now-next?backendId=home&channelId=S19.2E-1-100&fromTime=1791565200&limit=2";

ApiResponse read(const std::string& path, const std::string& actor,
    const std::vector<std::string>& scopes)
{
    ApiResponse answer;
    assert(PublicApiRuntime::instance().tryHandleGet(
        path, actor, "epg-request", "epg-correlation", answer,
        "", "", scopes));
    return answer;
}
}

int main()
{
    auto& runtime = PublicApiRuntime::instance();
    runtime.resetEpgNowNextLookup();
    const ApiResponse unavailable = read(Request,"actor",{"home"});
    assert(unavailable.statusCode == 503);
    assert(read(Request,"",{"home"}).statusCode == 401);
    assert(read(Request,"actor",{"foreign"}).statusCode == 400);
    assert(read(Request,"actor",{}).statusCode == 400);
    for (const std::string& invalid : {
        "/api/v1/epg/now-next?backendId=home&channelId=../private&fromTime=1791565200",
        "/api/v1/epg/now-next?backendId=home&channelId=S19.2E-1-100&fromTime=1791565200&limit=3",
        "/api/v1/epg/now-next?backendId=home&channelId=S19.2E-1-100&fromTime=1791565200&limit=2&limit=2",
        "/api/v1/epg/now-next?backendId=home&channelId=S19.2E-1-100&fromTime=1791565200&cursor=evil",
        "/api/v1/epg/now-next?backendId=home&channelId=S19.2E-1-100"
    }) assert(read(invalid,"actor",{"home"}).statusCode == 400);

    int called = 0;
    runtime.registerEpgNowNextLookup([&](const PublicEpgNowNextRequest& request) {
        ++called;
        assert(request.backendId == "home");
        assert(request.channelId == "S19.2E-1-100");
        assert(request.fromTime == "1791565200");
        assert(request.limit == 2U);
        PublicEpgNowNextResult result;
        result.status = PublicEpgNowNextStatus::ok;
        for (int index = 0; index < 2; ++index)
        {
            PublicEpgNowNextItem item;
            item.channelId = request.channelId;
            item.title = index == 0 ? "Heute & Morgen" : "Nachrichten";
            item.subtitle = "Jetzt";
            item.startTime = "1791565200";
            item.endTime = "1791568800";
            item.durationSeconds = 3600;
            result.events.push_back(item);
        }
        return result;
    });
    const ApiResponse ok = read(Request,"actor",{"home"});
    assert(ok.statusCode == 200);
    assert(called == 1);
    assert(ok.body.find("Heute & Morgen") != std::string::npos);
    assert(ok.body.find("\"durationSeconds\":3600") != std::string::npos);
    assert(ok.body.find("\"hasMore\":false") != std::string::npos);
    assert(ok.body.find("\"nativePath\"") == std::string::npos);
    assert(ok.headers.at("Cache-Control") == "no-store");

    runtime.registerEpgNowNextLookup([](const PublicEpgNowNextRequest& request) {
        PublicEpgNowNextResult result;
        result.status = PublicEpgNowNextStatus::ok;
        PublicEpgNowNextItem item;
        item.channelId = "other-backend-channel";
        item.title = "Leak";
        item.startTime = "1791565200";
        item.endTime = "1791568800";
        result.events.push_back(item);
        return result;
    });
    assert(read(Request,"actor",{"home"}).statusCode == 503);

    runtime.registerEpgNowNextLookup([](const PublicEpgNowNextRequest& request) {
        PublicEpgNowNextResult result;
        result.status = PublicEpgNowNextStatus::ok;
        result.events.resize(3);
        return result;
    });
    assert(read(Request,"actor",{"home"}).statusCode == 503);

    runtime.registerEpgNowNextLookup([](const PublicEpgNowNextRequest&) {
        PublicEpgNowNextResult result;
        result.status = PublicEpgNowNextStatus::notFound;
        return result;
    });
    assert(read(Request,"actor",{"home"}).statusCode == 404);

    // Complete, bounded EPG windows never use the legacy 2-event contract.
    const std::string epgWindow =
        "/api/v1/epg/window?backendId=home&channelId=S19.2E-1-100"
        "&fromTime=1791565200&untilTime=1791586800&limit=192";
    assert(read(epgWindow,"",{"home"}).statusCode == 401);
    assert(read(epgWindow,"actor",{"other"}).statusCode == 400);
    assert(read(epgWindow+"&limit=192","actor",{"home"}).statusCode == 400);
    assert(read("/api/v1/epg/window?backendId=home"
        "&channelId=S19.2E-1-100&fromTime=1791565200"
        "&untilTime=1791608401","actor",{"home"}).statusCode == 503);
    assert(read(epgWindow,"actor",{"home"}).statusCode == 503);
    runtime.registerEpgWindowLookup(
        [](const PublicEpgWindowRequest& request) {
            assert(request.backendId == "home");
            assert(request.channelId == "S19.2E-1-100");
            assert(request.fromTime == "1791565200");
            assert(request.untilTime == "1791586800");
            assert(request.limit == 192U);
            PublicEpgWindowResult result;
            result.status = PublicEpgNowNextStatus::ok;
            for (int i = 0; i != 3; ++i)
            {
                PublicEpgNowNextItem event;
                event.channelId = request.channelId;
                event.title = "Programm " + std::to_string(i);
                if (i == 0) event.eventId = "native-event:1234";
                if (i == 1) event.eventId = std::string(161U, 'x');
                if (i == 2) event.eventId = std::string(160U, 'n');
                event.startTime = std::to_string(1791565200LL + i*3600LL);
                event.endTime = std::to_string(1791568800LL + i*3600LL);
                event.durationSeconds = 3600;
                result.events.push_back(event);
            }
            result.hasMore = false;
            return result;
        });
    const auto windowOk = read(epgWindow,"actor",{"home"});
    assert(windowOk.statusCode == 200);
    assert(windowOk.body.find("Programm 2") != std::string::npos);
    assert(windowOk.body.find("\"eventId\":\"native-event:1234\"")
        != std::string::npos);
    assert(windowOk.body.find(std::string(161U, 'x')) ==
        std::string::npos);
    assert(windowOk.body.find(
        "\"eventId\":\"" + std::string(160U, 'n') + "\"") !=
        std::string::npos);
    const auto firstEventId = windowOk.body.find("\"eventId\"");
    assert(firstEventId != std::string::npos);
    const auto secondEventId =
        windowOk.body.find("\"eventId\"", firstEventId + 1U);
    assert(secondEventId != std::string::npos);
    assert(windowOk.body.find("\"eventId\"", secondEventId + 1U) ==
        std::string::npos);
    assert(windowOk.body.find("\"hasMore\":false") != std::string::npos);
    assert(windowOk.body.find("\"partial\":false") != std::string::npos);
    runtime.registerEpgWindowLookup(
        [](const PublicEpgWindowRequest& request) {
            PublicEpgWindowResult result;
            result.status = PublicEpgNowNextStatus::ok;
            PublicEpgNowNextItem event;
            event.channelId = "foreign";
            event.title = "No leak";
            event.startTime = "1791565200";
            event.endTime = "1791568800";
            result.events.push_back(event);
            return result;
        });
    assert(read(epgWindow,"actor",{"home"}).statusCode == 503);

    // E1: a multi-day window returns bounded deterministic pages, including
    // same-start entries and a cache-only identity which is never sent as
    // a native eventId in JSON.
    const std::string longWindow =
        "/api/v1/epg/window?backendId=home&channelId=S19.2E-1-100"
        "&fromTime=1791565200&untilTime=1791824400&limit=2";
    runtime.registerEpgWindowLookup(
        [](const PublicEpgWindowRequest& request) {
            PublicEpgWindowResult result;
            result.status = PublicEpgNowNextStatus::ok;
            const std::vector<std::string> startTimes = {
                "1791565100", "1791565200", "1791565200",
                "1791651600", "1791738000"};
            const std::vector<std::string> cacheIds = {
                "cache-0", "cache-1", "cache-2", "cache-3", "cache-4"};
            std::size_t position = 0U;
            if (!request.afterCacheEventId.empty())
            {
                assert(!request.afterStartTime.empty());
                while (position < cacheIds.size() &&
                       cacheIds[position] != request.afterCacheEventId)
                    ++position;
                assert(position < cacheIds.size());
                ++position;
            }
            const std::size_t end =
                std::min(cacheIds.size(), position + request.limit);
            for (std::size_t index = position; index < end; ++index)
            {
                PublicEpgNowNextItem event;
                event.channelId = request.channelId;
                event.title = "Programme-" + std::to_string(index);
                event.startTime = startTimes[index];
                event.endTime =
                    std::to_string(std::stoll(startTimes[index]) + 3600);
                event.durationSeconds = 3600;
                if (index != 2U) event.eventId = "native-" + cacheIds[index];
                result.events.push_back(event);
            }
            result.hasMore = end < cacheIds.size();
            if (result.hasMore)
            {
                result.nextStartTime = startTimes[end - 1U];
                result.nextCacheEventId = cacheIds[end - 1U];
            }
            return result;
        });
    const auto extractCursor = [](const std::string& body) {
        const std::string key = "\\\"nextCursor\\\":\\\"";
        const auto start = body.find(key);
        if (start == std::string::npos) return std::string();
        const auto from = start + key.size();
        const auto finish = body.find('\\\"', from);
        return finish == std::string::npos ?
            std::string() : body.substr(from, finish - from);
    };
    const auto page1 = read(longWindow, "actor", {"home"});
    assert(page1.statusCode == 200);
    assert(page1.body.find("Programme-0") != std::string::npos);
    assert(page1.body.find("Programme-1") != std::string::npos);
    assert(page1.body.find("Programme-2") == std::string::npos);
    assert(page1.body.find("\\\"hasMore\\\":true") != std::string::npos);
    const std::string cursor1 = extractCursor(page1.body);
    assert(cursor1.rfind("ew1_", 0U) == 0U);
    assert(read(longWindow + "&cursor=" + cursor1, "", {"home"})
        .statusCode == 401);
    assert(read(longWindow + "&cursor=" + cursor1, "actor", {"elsewhere"})
        .statusCode == 400);
    assert(read(longWindow + "&cursor=" + cursor1 + "&cursor=" + cursor1,
        "actor", {"home"}).statusCode == 400);
    assert(read(longWindow + "&cursor=malformed", "actor", {"home"})
        .statusCode == 400);
    assert(read(longWindow + "&cursor=" + cursor1 + "z", "actor", {"home"})
        .statusCode == 400);
    assert(read(
        "/api/v1/epg/window?backendId=other&channelId=S19.2E-1-100"
        "&fromTime=1791565200&untilTime=1791824400&limit=2&cursor=" +
            cursor1, "actor", {"other"}).statusCode == 400);
    assert(read(
        "/api/v1/epg/window?backendId=home&channelId=S19.2E-1-100"
        "&fromTime=1791565200&untilTime=1791824401&limit=2&cursor=" +
            cursor1, "actor", {"home"}).statusCode == 400);
    const auto page2 = read(longWindow + "&cursor=" + cursor1,
                            "actor", {"home"});
    assert(page2.statusCode == 200);
    assert(page2.body.find("Programme-2") != std::string::npos);
    assert(page2.body.find("Programme-3") != std::string::npos);
    assert(page2.body.find("Programme-1") == std::string::npos);
    assert(page2.body.find("cache-2") == std::string::npos);
    const std::string cursor2 = extractCursor(page2.body);
    assert(!cursor2.empty() && cursor2 != cursor1);
    const auto page3 = read(longWindow + "&cursor=" + cursor2,
                            "actor", {"home"});
    assert(page3.statusCode == 200);
    assert(page3.body.find("Programme-4") != std::string::npos);
    assert(page3.body.find("\\\"hasMore\\\":false") != std::string::npos);
    assert(page3.body.find("\\\"nextCursor\\\":null") != std::string::npos);
    // A window with no events remains valid when the cache is ready.
    runtime.registerEpgWindowLookup(
        [](const PublicEpgWindowRequest&) {
            PublicEpgWindowResult r;
            r.status = PublicEpgNowNextStatus::ok;
            return r;
        });
    assert(read(longWindow, "actor", {"home"}).statusCode == 200);
    assert(read(longWindow, "actor", {"home"}).body.find(
        "\\\"items\\\":[]") != std::string::npos);
    runtime.registerEpgWindowLookup(
        [](const PublicEpgWindowRequest& request) {
            PublicEpgWindowResult r;
            r.status = request.afterCacheEventId.empty()
                ? PublicEpgNowNextStatus::ok : PublicEpgNowNextStatus::invalid;
            if (r.status == PublicEpgNowNextStatus::ok)
            {
                PublicEpgNowNextItem item;
                item.channelId = request.channelId;
                item.title = "One";
                item.startTime = "1791565200";
                item.endTime = "1791568800";
                r.events.push_back(item);
                r.hasMore = true;
                r.nextStartTime = "1791565200";
                r.nextCacheEventId = "cache-key";
            }
            return r;
        });
    const std::string staleCursor = extractCursor(
        read(longWindow, "actor", {"home"}).body);
    assert(read(longWindow + "&cursor=" + staleCursor,
        "actor", {"home"}).statusCode == 400);
    runtime.resetEpgWindowLookup();

    // Real VDR timer snapshot resources must expose native data only.
    const std::string nativeTimers =
        "/api/v1/native-timers?backendId=home&offset=0&limit=30";
    assert(read(nativeTimers,"actor",{"home"}).statusCode == 503);
    assert(read(nativeTimers,"",{"home"}).statusCode == 401);
    assert(read(nativeTimers,"actor",{"other"}).statusCode == 400);
    assert(read(nativeTimers + "&limit=30","actor",{"home"})
        .statusCode == 400);
    assert(read("/api/v1/native-timers?backendId=home&offset=100001",
        "actor",{"home"}).statusCode == 400);
    runtime.registerNativeTimersLookup(
        [](const PublicNativeTimersRequest& request) {
            assert(request.backendId == "home");
            assert(request.offset == 0U);
            assert(request.limit == 30U);
            PublicNativeTimersResult result;
            result.status = PublicNativeTimersStatus::ok;
            PublicNativeTimerItem timer;
            timer.id = "42";
            timer.channelId = "S19.2E-1-100";
            timer.channelName = "Das Erste";
            timer.title = "Nachrichten";
            timer.day = "20261017";
            timer.weekdays = "-------";
            timer.startTime = "2000";
            timer.endTime = "2015";
            timer.enabled = true;
            result.timers.push_back(timer);
            return result;
        });
    const auto timers = read(nativeTimers,"actor",{"home"});
    assert(timers.statusCode == 200);
    assert(timers.body.find("\"timerId\":\"42\"") != std::string::npos);
    assert(timers.body.find("\"recording\":false") != std::string::npos);
    assert(timers.body.find("\"partial\":false") != std::string::npos);
    assert(timers.body.find("assignmentRevision") == std::string::npos);
    runtime.resetNativeTimersLookup();

    runtime.resetEpgNowNextLookup();
    return 0;
}
