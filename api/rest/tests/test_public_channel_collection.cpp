#include "PublicApiRuntime.h"
#include "PublicChannelNeighbor.h"

#include <algorithm>
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
    // Same production comparator used by daemon source pagination AND the
    // existing single-channel numeric neighbor, independent of string IDs.
    std::vector<PublicChannelCollectionItem> nativeChannels = vdr;
    std::sort(nativeChannels.begin(), nativeChannels.end(),
        publicVdrChannelNumberLess);
    assert(nativeChannels[0].channelId == "id-a");
    assert(nativeChannels[1].channelId == "id-m");
    assert(nativeChannels[2].channelId == "id-z");

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

            if (request.nativeChannelOrder)
            {
                // Deliberately scrambled channel IDs and duplicate VDR
                // positions: sort BEFORE slicing, tie-break by channelId.
                std::vector<PublicChannelCollectionItem> byNumber = {
                    channel("backend-a", "id-230", 230),
                    channel("backend-a", "id-17-z", 17),
                    channel("backend-a", "id-1", 1),
                    channel("backend-a", "id-17-a", 17)};
                std::sort(byNumber.begin(), byNumber.end(),
                    [](const PublicChannelCollectionItem& lhs,
                       const PublicChannelCollectionItem& rhs)
                    {
                        if (lhs.channelNumber != rhs.channelNumber)
                            return lhs.channelNumber < rhs.channelNumber;
                        return lhs.channelId < rhs.channelId;
                    });
                for (const auto& item : byNumber)
                {
                    if (request.afterBackendId.empty() ||
                        item.channelNumber > request.afterChannelNumber ||
                        (item.channelNumber == request.afterChannelNumber &&
                         item.channelId > request.afterChannelId))
                        result.channels.push_back(item);
                }
                result.hasMore = result.channels.size() > request.limit;
                if (result.hasMore) result.channels.resize(request.limit);
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

    // Preserve negative security/provenance behavior at the route boundary.
    ApiResponse anonymous;
    assert(runtime.tryHandleGet(
        "/api/v1/channels?backendId=backend-a",
        "", "channel-anonymous", "channel-correlation",
        anonymous, "", "", std::vector<std::string>{"backend-a"}));
    assert(anonymous.statusCode == 401);
    assert(anonymous.contentType == "application/problem+json");
    assert(anonymous.body.find("\"code\":\"unauthorized\"") !=
        std::string::npos);
    assert(anonymous.headers.at("X-Request-ID") == "channel-anonymous");

    ApiResponse missingScope;
    assert(runtime.tryHandleGet(
        "/api/v1/channels?backendId=backend-a",
        "actor:test", "channel-no-grant", "",
        missingScope, "", "", {}));
    assert(missingScope.statusCode == 400);
    assert(missingScope.body.find("\"code\":\"invalid_request\"") !=
        std::string::npos);

    ApiResponse duplicateScope;
    assert(runtime.tryHandleGet(
        "/api/v1/channels?backendId=backend-a&backendId=backend-a",
        "actor:test", "channel-duplicate-grant", "",
        duplicateScope, "", "",
        std::vector<std::string>{"backend-a", "backend-a"}));
    assert(duplicateScope.statusCode == 400);
    assert(duplicateScope.body.find("\"code\":\"invalid_request\"") !=
        std::string::npos);

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
    assert(first.headers.at("X-Request-ID") == "phase69d-channel-first");
    assert(first.headers.at("X-Correlation-ID") ==
        "phase69d-channel-correlation");
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

    // Additive numeric native channels.conf order with ch2_ paging. This
    // must select positions 1/17 on page one, not id-230.
    const std::string nativeBase =
        "/api/v1/channels?backendId=backend-a&limit=2"
        "&sort=backendId,channelNumber,channelId&order=asc";
    ApiResponse nativeFirst;
    assert(runtime.tryHandleGet(nativeBase,
        "actor:test", "channel-number-first", "", nativeFirst, "", "",
        std::vector<std::string>{"backend-a"}));
    assert(nativeFirst.statusCode == 200);
    assert(nativeFirst.body.find("\"channelId\":\"id-1\"") != std::string::npos);
    assert(nativeFirst.body.find("\"channelId\":\"id-17-a\"") != std::string::npos);
    assert(nativeFirst.body.find("\"channelId\":\"id-230\"") == std::string::npos);
    assert(nativeFirst.body.find("\"hasMore\":true") != std::string::npos);
    const std::string nativeCursor = nextCursor(nativeFirst.body);
    assert(nativeCursor.rfind("ch2_", 0U) == 0U);
    assert(nativeFirst.body.find("sort=backendId,channelNumber,channelId") !=
        std::string::npos);

    ApiResponse nativeSecond;
    assert(runtime.tryHandleGet(nativeBase + "&cursor=" + nativeCursor,
        "actor:test", "channel-number-second", "", nativeSecond, "", "",
        std::vector<std::string>{"backend-a"}));
    assert(nativeSecond.statusCode == 200);
    assert(nativeSecond.body.find("\"channelId\":\"id-17-z\"") != std::string::npos);
    assert(nativeSecond.body.find("\"channelId\":\"id-230\"") != std::string::npos);
    assert(nativeSecond.body.find("\"hasMore\":false") != std::string::npos);

    for (const std::string invalidMixed : {
         nativeBase + "&cursor=" + cursor,
         std::string("/api/v1/channels?backendId=backend-a&cursor=") + nativeCursor})
    {
        ApiResponse wrongSort;
        assert(runtime.tryHandleGet(invalidMixed,
            "actor:test", "channel-number-wrong-cursor", "", wrongSort, "", "",
            std::vector<std::string>{"backend-a"}));
        assert(wrongSort.statusCode == 400);
    }
    ApiResponse nativeScopeMismatch;
    assert(runtime.tryHandleGet(
        "/api/v1/channels?backendId=backend-c&limit=2"
        "&sort=backendId,channelNumber,channelId&order=asc&cursor=" + nativeCursor,
        "actor:test", "channel-number-scope", "", nativeScopeMismatch, "", "",
        std::vector<std::string>{"backend-c"}));
    assert(nativeScopeMismatch.statusCode == 409);

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
        assert(adjacent.body.find("&direction=") != std::string::npos);
        // A neighbor self-link must not carry the legacy sort/order fields,
        // because those fields are deliberately forbidden for neighbor reads.
        assert(adjacent.body.find("sort=backendId,channelId") ==
            std::string::npos);
    }
    for (const std::string invalid : {
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

    for (const std::string badQuery : {
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
