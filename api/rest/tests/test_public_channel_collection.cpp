#include "PublicApiRuntime.h"
#include "PublicChannelNeighbor.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string nextCursor(const std::string& body)
{
    const std::string marker = "\"nextCursor\":\"";
    const std::size_t start = body.find(marker);
    if (start == std::string::npos) return "";
    const std::size_t valueStart = start + marker.size();
    const std::size_t end = body.find('"', valueStart);
    if (end == std::string::npos) return "";
    return body.substr(valueStart, end - valueStart);
}

PublicChannelCollectionItem channel(
    const std::string& backendId,
    const std::string& channelId,
    int number)
{
    PublicChannelCollectionItem item;
    item.backendId = backendId;
    item.channelId = channelId;
    item.channelNumber = number;
    item.name = "Channel " + channelId;
    item.provider = "provider";
    item.groupName = "group";
    item.enabled = true;
    return item;
}
}

int main()
{
    // Real pure resolver used by DaemonRuntimeInitialization: test VDR
    // numeric order, wrap, disabled/radio skipping and no invented neighbor.
    std::vector<PublicChannelCollectionItem> vdr = {
        channel("backend-a", "id-z", 30),
        channel("backend-a", "id-a", 10),
        channel("backend-a", "id-m", 20),
        channel("backend-a", "disabled", 40),
        channel("backend-a", "radio", 50)};
    vdr[3].enabled = false;
    vdr[4].radio = true;
    const auto next = findPublicVdrChannelNeighbor(vdr, "id-a", "up");
    const auto previous = findPublicVdrChannelNeighbor(vdr, "id-a", "down");
    assert(next && next->channelId == "id-m");
    assert(previous && previous->channelId == "id-z");
    assert(findPublicVdrChannelNeighbor(vdr, "id-z", "up")->channelId == "id-a");
    assert(findPublicVdrChannelNeighbor(vdr, "id-m", "down")->channelId == "id-a");
    assert(!findPublicVdrChannelNeighbor(vdr, "missing", "up"));
    assert(!findPublicVdrChannelNeighbor(vdr, "disabled", "up"));
    assert(!findPublicVdrChannelNeighbor(vdr, "radio", "down"));
    assert(!findPublicVdrChannelNeighbor(vdr, "id-a", "sideways"));
    vdr.resize(1U);
    assert(!findPublicVdrChannelNeighbor(vdr, "id-z", "up"));

    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetChannelCollectionLookup();

    runtime.registerChannelCollectionLookup(
        [](const PublicChannelCollectionRequest& request)
        {
            PublicChannelCollectionResult result;
            if (request.backendIds ==
                std::vector<std::string>{"backend-down"})
            {
                result.status =
                    PublicChannelCollectionStatus::
                        allSourcesUnavailable;
                return result;
            }

            result.status = PublicChannelCollectionStatus::ok;
            for (const std::string& backendId : request.backendIds)
            {
                PublicChannelCollectionSource source;
                source.backendId = backendId;
                if (backendId == "backend-b")
                {
                    source.state = "unavailable";
                    source.code = "backend_unavailable";
                }
                else
                {
                    source.state = "ok";
                }
                result.sources.push_back(source);
            }

            if (!request.relativeToChannelId.empty())
            {
                // Intentionally NOT ID order: VDR numbers are the authority.
                const std::vector<PublicChannelCollectionItem> vdr = {
                    channel("backend-a", "S19.2E-1-1019-10303", 7),
                    channel("backend-a", "S19.2E-1-1019-10301", 2),
                    channel("backend-a", "S19.2E-1-1019-10302", 5)};
                std::size_t index = vdr.size();
                for (std::size_t i = 0U; i < vdr.size(); ++i)
                    if (vdr[i].channelId == request.relativeToChannelId)
                        index = i;
                if (index == vdr.size() || request.backendIds.size() != 1U ||
                    request.backendIds.front() != "backend-a")
                {
                    result.status = PublicChannelCollectionStatus::notFound;
                    return result;
                }
                const std::size_t adjacent = request.direction == "up" ?
                    (index + 1U) % vdr.size() :
                    (index + vdr.size() - 1U) % vdr.size();
                result.channels = {vdr[adjacent]};
                result.hasMore = false;
                return result;
            }

            const std::vector<PublicChannelCollectionItem> available = {
                channel("backend-a", "S19.2E-1-1019-10301", 1),
                channel("backend-a", "S19.2E-1-1019-10302", 2),
                channel("backend-c", "S19.2E-1-1101-28106", 3)};
            std::vector<PublicChannelCollectionItem> eligible;
            for (const auto& item : available)
            {
                if (request.afterBackendId.empty() ||
                    item.backendId > request.afterBackendId ||
                    (item.backendId == request.afterBackendId &&
                     item.channelId > request.afterChannelId))
                    eligible.push_back(item);
            }
            result.hasMore = eligible.size() > request.limit;
            const std::size_t count =
                result.hasMore ? request.limit : eligible.size();
            result.channels.assign(
                eligible.begin(),
                eligible.begin() +
                    static_cast<std::ptrdiff_t>(count));
            return result;
        });

    const std::vector<std::string> scopes = {
        "backend-a", "backend-b", "backend-c"};

    ApiResponse first;
    assert(runtime.tryHandleGet(
        "/api/v1/channels?backendId=backend-c&backendId=backend-a&backendId=backend-b&limit=2",
        "actor:test",
        "phase69d-channel-first",
        "phase69d-channel-correlation",
        first,
        "",
        "",
        scopes));
    assert(first.statusCode == 200);
    assert(first.body.find(
        "\"channelId\":\"S19.2E-1-1019-10301\"") !=
        std::string::npos);
    assert(first.body.find("\"partial\":true") !=
        std::string::npos);
    assert(first.body.find(
        "\"backendId\":\"backend-b\",\"state\":\"unavailable\",\"code\":\"backend_unavailable\"") !=
        std::string::npos);
    assert(first.body.find("\"hasMore\":true") !=
        std::string::npos);
    assert(first.headers.at("Cache-Control") == "no-store");
    assert(first.headers.find("ETag") == first.headers.end());

    const std::string cursor = nextCursor(first.body);
    assert(!cursor.empty());
    assert(cursor.rfind("ch1_", 0U) == 0U);

    ApiResponse second;
    assert(runtime.tryHandleGet(
        std::string(
            "/api/v1/channels?backendId=backend-a&backendId=backend-b&backendId=backend-c&limit=2&cursor=") +
            cursor,
        "actor:test",
        "phase69d-channel-second",
        "",
        second,
        "",
        "",
        scopes));
    assert(second.statusCode == 200);
    assert(second.body.find(
        "\"backendId\":\"backend-c\"") !=
        std::string::npos);
    assert(second.body.find("\"hasMore\":false") !=
        std::string::npos);

    // The old ch1_ pagination above is kept byte-compatible. A separate
    // single-row operation navigates real VDR number ordering with wrap.
    for (const auto& sample : std::vector<std::pair<std::string, std::string>>{
             {"10301&direction=up", "10302"},
             {"10303&direction=up", "10301"},
             {"10301&direction=down", "10303"}})
    {
        ApiResponse adjacent;
        const std::string from =
            "S19.2E-1-1019-" + sample.first;
        assert(runtime.tryHandleGet(
            "/api/v1/channels?backendId=backend-a&limit=1&relativeTo=" +
                from,
            "actor:test", "channel-neighbor", "", adjacent, "", "",
            std::vector<std::string>{"backend-a"}));
        assert(adjacent.statusCode == 200);
        assert(adjacent.body.find(
            "\"channelId\":\"S19.2E-1-1019-" + sample.second + "\"") !=
            std::string::npos);
        assert(adjacent.body.find("\"hasMore\":false") !=
            std::string::npos);
        assert(adjacent.body.find("\"nextCursor\":null") !=
            std::string::npos);
    }
    for (const std::string& invalid : {
             "relativeTo=S19.2E-1-1019-10301",
             "direction=up&limit=1",
             "relativeTo=S19.2E-1-1019-10301&direction=up&limit=30",
             "relativeTo=S19.2E-1-1019-10301&direction=up&limit=1&cursor=ch1_bad",
             "relativeTo=S19.2E-1-1019-10301&direction=other&limit=1",
             "relativeTo=../../etc&direction=up&limit=1"})
    {
        ApiResponse invalidNeighbor;
        assert(runtime.tryHandleGet(
            "/api/v1/channels?backendId=backend-a&" + invalid,
            "actor:test", "channel-neighbor-invalid", "",
            invalidNeighbor, "", "",
            std::vector<std::string>{"backend-a"}));
        assert(invalidNeighbor.statusCode == 400);
    }
    ApiResponse missingNeighbor;
    assert(runtime.tryHandleGet(
        "/api/v1/channels?backendId=backend-a&limit=1&relativeTo=missing&direction=up",
        "actor:test", "channel-neighbor-missing", "",
        missingNeighbor, "", "",
        std::vector<std::string>{"backend-a"}));
    assert(missingNeighbor.statusCode == 404);

    ApiResponse changedScope;
    assert(runtime.tryHandleGet(
        std::string(
            "/api/v1/channels?backendId=backend-a&backendId=backend-c&cursor=") +
            cursor,
        "actor:test",
        "phase69d-channel-scope-change",
        "",
        changedScope,
        "",
        "",
        std::vector<std::string>{"backend-a", "backend-c"}));
    assert(changedScope.statusCode == 409);
    assert(changedScope.body.find(
        "\"code\":\"cursor_expired\"") !=
        std::string::npos);

    ApiResponse malformed;
    assert(runtime.tryHandleGet(
        "/api/v1/channels?backendId=backend-a&cursor=broken",
        "actor:test",
        "phase69d-channel-bad-cursor",
        "",
        malformed,
        "",
        "",
        std::vector<std::string>{"backend-a"}));
    assert(malformed.statusCode == 400);

    ApiResponse allUnavailable;
    assert(runtime.tryHandleGet(
        "/api/v1/channels?backendId=backend-down",
        "actor:test",
        "phase69d-channel-down",
        "",
        allUnavailable,
        "",
        "",
        std::vector<std::string>{"backend-down"}));
    assert(allUnavailable.statusCode == 503);
    assert(allUnavailable.body.find(
        "\"code\":\"backend_unavailable\"") !=
        std::string::npos);

    for (const std::string& badQuery : {
             "limit=0",
             "limit=101",
             "sort=channelId",
             "order=desc",
             "offset=1"})
    {
        ApiResponse bad;
        assert(runtime.tryHandleGet(
            std::string(
                "/api/v1/channels?backendId=backend-a&") +
                badQuery,
            "actor:test",
            "phase69d-channel-bad-query",
            "",
            bad,
            "",
            "",
            std::vector<std::string>{"backend-a"}));
        assert(bad.statusCode == 400);
    }

    ApiResponse countMismatch;
    assert(runtime.tryHandleGet(
        "/api/v1/channels?backendId=backend-a",
        "actor:test",
        "phase69d-channel-scope-count",
        "",
        countMismatch,
        "",
        "",
        std::vector<std::string>{"backend-a", "backend-c"}));
    assert(countMismatch.statusCode == 400);

    ApiResponse post;
    assert(runtime.tryHandlePost(
        "/api/v1/channels?backendId=backend-a",
        "phase69d-channel-post",
        "",
        post));
    assert(post.statusCode == 405);
    assert(post.headers.at("Allow") == "GET");

    runtime.resetChannelCollectionLookup();
    return 0;
}
