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
    assert(ok.body.find("\"label\":\"Die Herrschaft des Feuers\"") !=
           std::string::npos);
    assert(ok.body.find("\"label\":\"Zweiter Film\"") !=
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
    // Three bounded pages, no path disclosure and no import side effect.
    // Continuation is bound to the exact actor/backend/source observation.
    std::vector<std::string> many;
    for (int i = 0; i < 70; ++i)
        many.push_back("source_" + std::to_string(10000000000ULL + i));
    api.registerMediaImportSourceLookup(
        [&many](const std::string& actor, const std::string& backend)
        {
            PublicApiRuntime::MediaImportSourceResult result;
            if (actor != "actor:one" || backend != "backend-a") {
                result.status = PublicApiRuntime::MediaImportSourceStatus::forbidden;
                return result;
            }
            result.status = PublicApiRuntime::MediaImportSourceStatus::ok;
            result.ids = many;
            return result;
        });
    const auto getPage = [&](const std::string& target,
                             const std::string& actor = "actor:one")
    {
        ApiResponse response;
        assert(api.tryHandleGet(target, actor, "page", "",
                                response, "", "", scopes));
        return response;
    };
    const auto nextFrom = [](const std::string& body) {
        const std::string name = "\"next\":\"";
        const auto start = body.find(name);
        if (start == std::string::npos) return std::string();
        const auto offset = start + name.size();
        const auto end = body.find('"', offset);
        assert(end != std::string::npos);
        return body.substr(offset, end - offset);
    };
    auto first = getPage(url);
    assert(first.statusCode == 200);
    assert(first.body.find("\"meta\":{\"partial\":true}") != std::string::npos);
    assert(first.body.find(many[0]) != std::string::npos);
    assert(first.body.find(many[31]) != std::string::npos);
    assert(first.body.find(many[32]) == std::string::npos);
    const std::string secondUrl = nextFrom(first.body);
    assert(secondUrl.find(url + "?cursor=mi1_") == 0);
    auto second = getPage(secondUrl);
    assert(second.statusCode == 200);
    assert(second.body.find(many[32]) != std::string::npos);
    assert(second.body.find(many[63]) != std::string::npos);
    const std::string thirdUrl = nextFrom(second.body);
    assert(!thirdUrl.empty());
    auto third = getPage(thirdUrl);
    assert(third.statusCode == 200);
    assert(third.body.find(many[64]) != std::string::npos);
    assert(third.body.find(many[69]) != std::string::npos);
    assert(nextFrom(third.body).empty());
    assert(getPage(secondUrl, "actor:two").statusCode == 403);
    many[0] = "source_09999999999"; // valid new snapshot, old continuation stale
    assert(getPage(secondUrl).statusCode == 409);
    assert(getPage(url + "?cursor=mi1_bad_32").statusCode == 409);
    assert(getPage(url + "?cursor=mi1_1_00").statusCode == 409);
    assert(getPage(url + "?cursor=mi1_1_32&path=/tmp").statusCode == 400);
    assert(getPage(url + "?cursor=").statusCode == 400);
    many.resize(257, "source_unreachable_00000000000");
    assert(getPage(url).statusCode == 503);
    ApiResponse post;
    assert(api.tryHandlePost(url, "", "", post));
    assert(post.statusCode == 405);
    ApiResponse patch;
    assert(api.tryHandleUnsupportedMethod("PATCH", url, "", "", patch));
    assert(patch.statusCode == 405);
    api.resetMediaImportSourceLookup();
}
