#include "PublicApiRuntime.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
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

PublicBackendCollectionItem backend(
    const std::string& id,
    const std::string& name,
    const std::string& type,
    bool enabled,
    bool online)
{
    PublicBackendCollectionItem item;
    item.backendId = id;
    item.name = name;
    item.type = type;
    item.enabled = enabled;
    item.online = online;
    return item;
}

bool authorized(
    const std::vector<std::string>& scopes,
    const std::string& backendId)
{
    return std::find(scopes.begin(), scopes.end(), "*") != scopes.end() ||
        std::find(scopes.begin(), scopes.end(), backendId) != scopes.end();
}
}

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetBackendCollectionLookup();

    const std::vector<PublicBackendCollectionItem> configured = {
        backend("backend-a", "Living Room", "restfulapi", true, true),
        backend("backend-b", "Bedroom", "agent", true, false),
        backend("backend-c", "Remote House", "agent", false, false)};

    runtime.registerBackendCollectionLookup(
        [configured](const PublicBackendCollectionRequest& request)
        {
            PublicBackendCollectionResult result;
            result.status = PublicBackendCollectionStatus::ok;

            std::vector<PublicBackendCollectionItem> eligible;
            for (const auto& item : configured)
            {
                if (!authorized(request.authorizedBackendIds, item.backendId))
                    continue;
                if (!request.afterBackendId.empty() &&
                    item.backendId <= request.afterBackendId)
                    continue;
                eligible.push_back(item);
            }

            result.hasMore = eligible.size() > request.limit;
            if (result.hasMore)
                eligible.resize(request.limit);
            result.backends = std::move(eligible);
            return result;
        });

    ApiResponse root;
    assert(runtime.tryHandleGet(
        "/api/v1",
        "actor:test",
        "phase69f-backends-root",
        "",
        root));
    assert(root.statusCode == 200);
    assert(root.body.find(
        "\"backends\":\"/api/v1/backends\"") != std::string::npos);

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor:test",
        "phase69f-backends-capabilities",
        "",
        capabilities));
    assert(capabilities.statusCode == 200);
    assert(capabilities.body.find(
        "\"id\":\"public-api.backends-read\"") != std::string::npos);
    assert(capabilities.body.find(
        "\"availability\":\"available\"") != std::string::npos);

    ApiResponse first;
    assert(runtime.tryHandleGet(
        "/api/v1/backends?limit=1&sort=backendId&order=asc",
        "actor:test",
        "phase69f-backends-first",
        "phase69f-backends-correlation",
        first,
        "",
        "",
        std::vector<std::string>{"backend-a", "backend-c"}));
    assert(first.statusCode == 200);
    assert(first.body.find("\"backendId\":\"backend-a\"") != std::string::npos);
    assert(first.body.find("\"backendId\":\"backend-b\"") == std::string::npos);
    assert(first.body.find("\"name\":\"Living Room\"") != std::string::npos);
    assert(first.body.find("\"type\":\"restfulapi\"") != std::string::npos);
    assert(first.body.find("\"enabled\":true") != std::string::npos);
    assert(first.body.find("\"online\":true") != std::string::npos);
    assert(first.body.find("frontendSelector") == std::string::npos);
    assert(first.body.find("accessMode") == std::string::npos);
    assert(first.body.find("capabilities") == std::string::npos);
    assert(first.body.find("canWrite") == std::string::npos);
    assert(first.body.find("\"partial\":false") != std::string::npos);
    assert(first.body.find("\"hasMore\":true") != std::string::npos);
    assert(first.headers.at("Cache-Control") == "no-store");
    assert(first.headers.find("ETag") == first.headers.end());

    const std::string cursor = nextCursor(first.body);
    assert(!cursor.empty());
    assert(cursor.rfind("be1_", 0U) == 0U);

    ApiResponse second;
    assert(runtime.tryHandleGet(
        std::string("/api/v1/backends?limit=1&cursor=") + cursor,
        "actor:test",
        "phase69f-backends-second",
        "",
        second,
        "",
        "",
        std::vector<std::string>{"backend-a", "backend-c"}));
    assert(second.statusCode == 200);
    assert(second.body.find("\"backendId\":\"backend-c\"") != std::string::npos);
    assert(second.body.find("\"hasMore\":false") != std::string::npos);

    ApiResponse changedScope;
    assert(runtime.tryHandleGet(
        std::string("/api/v1/backends?limit=1&cursor=") + cursor,
        "actor:test",
        "phase69f-backends-scope-change",
        "",
        changedScope,
        "",
        "",
        std::vector<std::string>{"backend-a", "backend-b", "backend-c"}));
    assert(changedScope.statusCode == 409);
    assert(changedScope.body.find("\"code\":\"cursor_expired\"") != std::string::npos);

    ApiResponse wildcard;
    assert(runtime.tryHandleGet(
        "/api/v1/backends?limit=50",
        "actor:test",
        "phase69f-backends-wildcard",
        "",
        wildcard,
        "",
        "",
        std::vector<std::string>{"*"}));
    assert(wildcard.statusCode == 200);
    assert(wildcard.body.find("\"backendId\":\"backend-a\"") != std::string::npos);
    assert(wildcard.body.find("\"backendId\":\"backend-b\"") != std::string::npos);
    assert(wildcard.body.find("\"backendId\":\"backend-c\"") != std::string::npos);

    ApiResponse emptyScope;
    assert(runtime.tryHandleGet(
        "/api/v1/backends",
        "actor:test",
        "phase69f-backends-empty",
        "",
        emptyScope,
        "",
        "",
        {}));
    assert(emptyScope.statusCode == 200);
    assert(emptyScope.body.find("\"items\":[]") != std::string::npos);

    ApiResponse malformed;
    assert(runtime.tryHandleGet(
        "/api/v1/backends?cursor=broken",
        "actor:test",
        "phase69f-backends-bad-cursor",
        "",
        malformed,
        "",
        "",
        std::vector<std::string>{"backend-a"}));
    assert(malformed.statusCode == 400);

    for (const std::string& badQuery : {
             "limit=0",
             "limit=101",
             "sort=name",
             "order=desc",
             "offset=1",
             "backendId=backend-a"})
    {
        ApiResponse bad;
        assert(runtime.tryHandleGet(
            std::string("/api/v1/backends?") + badQuery,
            "actor:test",
            "phase69f-backends-bad-query",
            "",
            bad,
            "",
            "",
            std::vector<std::string>{"backend-a"}));
        assert(bad.statusCode == 400);
    }

    runtime.resetBackendCollectionLookup();
    ApiResponse unavailable;
    assert(runtime.tryHandleGet(
        "/api/v1/backends",
        "actor:test",
        "phase69f-backends-unavailable",
        "",
        unavailable,
        "",
        "",
        std::vector<std::string>{"backend-a"}));
    assert(unavailable.statusCode == 503);

    ApiResponse post;
    assert(runtime.tryHandlePost(
        "/api/v1/backends",
        "phase69f-backends-post",
        "",
        post));
    assert(post.statusCode == 405);
    assert(post.headers.at("Allow") == "GET");

    return 0;
}
