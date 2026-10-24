#include "PublicApiRuntime.h"
#include <cassert>
#include <string>
#include <vector>
int main()
{
    auto& api = PublicApiRuntime::instance();
    api.resetMediaImportSourceLookup();
    const std::string url =
        "/api/v1/backends/backend-a/media-import-sources";
    const std::vector<std::string> scopes{"backend-a"};
    auto get = [&](const std::string& actor,
                   const std::vector<std::string>& granted)
    {
        ApiResponse response;
        assert(api.tryHandleGet(url, actor, "list", "", response,
                                "", "", granted));
        return response;
    };
    assert(get("actor:one", scopes).statusCode == 503);
    api.registerMediaImportSourceLookup(
        [](const std::string& actor, const std::string& backend)
        {
            PublicApiRuntime::MediaImportSourceResult r;
            if (actor != "actor:one" || backend != "backend-a")
            {
                r.status = PublicApiRuntime::MediaImportSourceStatus::forbidden;
                return r;
            }
            r.status = PublicApiRuntime::MediaImportSourceStatus::ok;
            r.ids = {"source_00000000001", "source_00000000002"};
            r.labels = {"Die Herrschaft des Feuers", "Zweiter Film"};
            return r;
        });
    const auto ok = get("actor:one", scopes);
    assert(ok.statusCode == 200);
    assert(ok.body.find("\"sourceId\":\"source_00000000001\"") !=
           std::string::npos);
    assert(ok.body.find("\"sourceId\":\"source_00000000002\"") !=
           std::string::npos);
    assert(ok.body.find("/media/input") == std::string::npos);
    assert(ok.headers.at("Cache-Control") == "no-store");
    assert(get("actor:two", scopes).statusCode == 403);
    assert(get("actor:one", {"backend-b"}).statusCode == 404);
    assert(get("", scopes).statusCode == 401);
    ApiResponse query;
    assert(api.tryHandleGet(url + "?path=/tmp", "actor:one", "", "",
                            query, "", "", scopes));
    assert(query.statusCode == 400);
    ApiResponse post;
    assert(api.tryHandlePost(url, "", "", post));
    assert(post.statusCode == 405);
    ApiResponse patch;
    assert(api.tryHandleUnsupportedMethod("PATCH", url, "", "", patch));
    assert(patch.statusCode == 405);
    api.resetMediaImportSourceLookup();
}
