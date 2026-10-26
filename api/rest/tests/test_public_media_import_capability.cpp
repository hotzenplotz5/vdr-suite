#include "PublicApiRuntime.h"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    PublicApiRuntime& api = PublicApiRuntime::instance();
    api.resetMediaImportCapabilityLookup();
    const std::string path =
        "/api/v1/backends/backend-a/capabilities/media-import";
    const std::vector<std::string> scopes{"backend-a"};
    ApiResponse missing;
    assert(api.tryHandleGet(path, "actor:alice", "a7-missing", "", missing,
                            "", "", scopes));
    assert(missing.statusCode == 200);
    assert(missing.body.find("\"available\":false") != std::string::npos);
    assert(missing.headers.at("Cache-Control") == "no-store");

    int checked = 0;
    api.registerMediaImportCapabilityLookup(
        [&](const std::string& actor, const std::string& backend)
        {
            ++checked;
            return actor == "actor:alice" && backend == "backend-a";
        });

    ApiResponse success;
    assert(api.tryHandleGet(path, "actor:alice", "a7-success", "corr",
                            success, "", "", scopes));
    assert(success.statusCode == 200);
    assert(success.body.find("\"backendId\":\"backend-a\"") != std::string::npos);
    assert(success.body.find("\"capability\":\"addons.media.import\"") != std::string::npos);
    assert(success.body.find("\"available\":true") != std::string::npos);
    assert(success.headers.at("X-Request-ID") == "a7-success");
    assert(success.headers.at("X-Correlation-ID") == "corr");
    assert(checked == 1);

    ApiResponse noGrant;
    assert(api.tryHandleGet(path, "actor:bob", "a7-grant", "",
                            noGrant, "", "", scopes));
    assert(noGrant.statusCode == 200);
    assert(noGrant.body.find("\"available\":false") != std::string::npos);
    assert(checked == 2);

    ApiResponse outOfScope;
    assert(api.tryHandleGet(path, "actor:alice", "a7-out-scope", "",
                            outOfScope, "", "",
                            std::vector<std::string>{"backend-b"}));
    assert(outOfScope.statusCode == 404);
    assert(checked == 2);

    ApiResponse noScope;
    assert(api.tryHandleGet(path, "actor:alice", "a7-empty-scope", "",
                            noScope));
    assert(noScope.statusCode == 404);
    assert(checked == 2);

    ApiResponse anonymous;
    assert(api.tryHandleGet(path, "", "a7-anonymous", "", anonymous,
                            "", "", scopes));
    assert(anonymous.statusCode == 401);
    assert(checked == 2);

    ApiResponse invalidQuery;
    assert(api.tryHandleGet(path + "?source=/tmp/test", "actor:alice",
                            "a7-query", "", invalidQuery, "", "", scopes));
    assert(invalidQuery.statusCode == 400);
    assert(checked == 2);

    ApiResponse wildcard;
    assert(api.tryHandleGet(path, "actor:alice", "a7-wildcard", "",
                            wildcard, "", "",
                            std::vector<std::string>{"*"}));
    assert(wildcard.statusCode == 200);
    assert(wildcard.body.find("\"available\":true") != std::string::npos);
    assert(checked == 3);

    ApiResponse segment;
    assert(api.tryHandleGet(
        "/api/v1/backends/backend-a/extra/capabilities/media-import",
        "actor:alice", "a7-segment", "", segment, "", "", scopes));
    assert(segment.statusCode == 404);
    assert(checked == 3);

    ApiResponse encoded;
    assert(api.tryHandleGet(
        "/api/v1/backends/backend%2Fa/capabilities/media-import",
        "actor:alice", "a7-encoded", "", encoded, "", "", scopes));
    assert(encoded.statusCode == 404);
    assert(checked == 3);

    ApiResponse post;
    assert(api.tryHandlePost(path, "a7-post", "", post));
    assert(post.statusCode == 405);
    assert(post.headers.at("Allow") == "GET");

    ApiResponse patch;
    assert(api.tryHandleUnsupportedMethod("PATCH", path, "a7-patch",
                                          "", patch));
    assert(patch.statusCode == 405);
    assert(patch.headers.at("Allow") == "GET");

    api.resetMediaImportCapabilityLookup();
    return 0;
}
