#include "PublicApiRuntime.h"

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

PublicAccountCollectionItem account(
    const std::string& accountId,
    const std::string& actorId,
    const std::string& displayName,
    bool active)
{
    PublicAccountCollectionItem item;
    item.accountId = accountId;
    item.actorId = actorId;
    item.displayName = displayName;
    item.active = active;
    return item;
}
}

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetAccountCollectionLookup();

    const std::vector<PublicAccountCollectionItem> configured = {
        account("account-a", "actor-a", "Admin A", true),
        account("account-b", "actor-b", "User B", true),
        account("account-c", "actor-c", "Disabled C", false)};

    runtime.registerAccountCollectionLookup(
        [configured](const PublicAccountCollectionRequest& request)
        {
            PublicAccountCollectionResult result;
            result.status = PublicAccountCollectionStatus::ok;

            std::vector<PublicAccountCollectionItem> eligible;
            for (const auto& item : configured)
            {
                if (!request.afterAccountId.empty() &&
                    item.accountId <= request.afterAccountId)
                {
                    continue;
                }
                eligible.push_back(item);
            }

            result.hasMore = eligible.size() > request.limit;
            if (result.hasMore)
                eligible.resize(request.limit);
            result.accounts = std::move(eligible);
            return result;
        });

    ApiResponse root;
    assert(runtime.tryHandleGet(
        "/api/v1",
        "actor:test",
        "p2-accounts-root",
        "",
        root));
    assert(root.statusCode == 200);
    assert(root.body.find(
        "\"accounts\":\"/api/v1/accounts\"") != std::string::npos);

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor:test",
        "p2-accounts-capabilities",
        "",
        capabilities));
    assert(capabilities.statusCode == 200);
    assert(capabilities.body.find(
        "\"id\":\"public-api.accounts-read\"") != std::string::npos);
    assert(capabilities.body.find(
        "{\"id\":\"public-api.accounts-read\",\"version\":1,\"availability\":\"available\"}") !=
        std::string::npos);

    ApiResponse first;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts?limit=1&sort=accountId&order=asc",
        "actor:test",
        "p2-accounts-first",
        "p2-accounts-correlation",
        first));
    assert(first.statusCode == 200);
    assert(first.body.find("\"accountId\":\"account-a\"") != std::string::npos);
    assert(first.body.find("\"actorId\":\"actor-a\"") != std::string::npos);
    assert(first.body.find("\"displayName\":\"Admin A\"") != std::string::npos);
    assert(first.body.find("\"active\":true") != std::string::npos);
    assert(first.body.find("\"accountId\":\"account-b\"") == std::string::npos);
    assert(first.body.find("password") == std::string::npos);
    assert(first.body.find("credential") == std::string::npos);
    assert(first.body.find("session") == std::string::npos);
    assert(first.body.find("grant") == std::string::npos);
    assert(first.body.find("\"partial\":false") != std::string::npos);
    assert(first.body.find("\"hasMore\":true") != std::string::npos);
    assert(first.headers.at("Cache-Control") == "no-store");
    assert(first.headers.find("ETag") == first.headers.end());

    const std::string cursor = nextCursor(first.body);
    assert(!cursor.empty());
    assert(cursor.rfind("ac1_", 0U) == 0U);

    ApiResponse second;
    assert(runtime.tryHandleGet(
        std::string("/api/v1/accounts?limit=1&cursor=") + cursor,
        "actor:test",
        "p2-accounts-second",
        "",
        second));
    assert(second.statusCode == 200);
    assert(second.body.find("\"accountId\":\"account-b\"") != std::string::npos);

    ApiResponse malformed;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts?cursor=broken",
        "actor:test",
        "p2-accounts-bad-cursor",
        "",
        malformed));
    assert(malformed.statusCode == 400);

    for (const std::string& badQuery : {
             "limit=0",
             "limit=101",
             "sort=displayName",
             "order=desc",
             "offset=1",
             "backendId=default"})
    {
        ApiResponse bad;
        assert(runtime.tryHandleGet(
            std::string("/api/v1/accounts?") + badQuery,
            "actor:test",
            "p2-accounts-bad-query",
            "",
            bad));
        assert(bad.statusCode == 400);
    }

    runtime.resetAccountCollectionLookup();
    ApiResponse unavailable;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts",
        "actor:test",
        "p2-accounts-unavailable",
        "",
        unavailable));
    assert(unavailable.statusCode == 503);

    ApiResponse post;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts",
        "p2-accounts-post",
        "",
        post));
    assert(post.statusCode == 405);
    assert(post.headers.at("Allow") == "GET");

    for (const std::string& method :
         {std::string("PUT"),
          std::string("PATCH"),
          std::string("DELETE"),
          std::string("HEAD"),
          std::string("OPTIONS")})
    {
        ApiResponse mismatch;
        assert(runtime.tryHandleUnsupportedMethod(
            method,
            "/api/v1/accounts",
            "p2-accounts-method",
            "",
            mismatch));
        assert(mismatch.statusCode == 405);
        assert(mismatch.headers.at("Allow") == "GET");
    }

    return 0;
}
