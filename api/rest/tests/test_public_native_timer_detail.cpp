#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    auto& api = PublicApiRuntime::instance();
    api.resetNativeTimerDetailLookup();
    const std::string uri = "/api/v1/native-timers/17?backendId=backend-one";
    auto get = [&](const std::string& path,
        const std::string& ifNoneMatch = std::string(),
        const std::string& actor = "viewer",
        const std::string& backend = "backend-one")
    {
        ApiResponse r;
        const std::vector<std::string> scopes =
            backend.empty() ? std::vector<std::string>{} :
                              std::vector<std::string>{backend};
        assert(api.tryHandleGet(path, actor, "request", "correlation", r,
            ifNoneMatch, backend, scopes));
        return r;
    };

    assert(get(uri).statusCode == 503);
    bool bound = true;
    std::string currentAux = R"xml(<epgsearch note="quoted">original</epgsearch>)xml";
    auto status = PublicNativeTimerDetailStatus::ok;
    api.registerNativeTimerDetailLookup([&](const PublicNativeTimerDetailRequest& req)
    {
        assert(req.backendId == "backend-one" && req.timerId == "17");
        PublicNativeTimerDetailResult r;
        r.status = status;
        r.backendId = req.backendId;
        r.timer.id = req.timerId;
        r.timer.title = "Tagesschau";
        r.timer.aux = currentAux;
        r.timer.eventId = "117721";
        r.timer.flags = 5;
        r.timer.endTime = "2030";
        r.timer.directory = "News~Tagesschau";
        r.timer.priority = 68;
        r.timer.lifetime = 19;
        r.timer.vps = true;
        if (bound)
        {
            r.resourceRevision = "native-timer-binding/1:bind:3:8";
            r.bindingState = "verified";
            r.ownership = "managed";
        }
        return r;
    });
    const auto ok = get(uri);
    assert(ok.statusCode == 200);
    assert(ok.body.find("Tagesschau") != std::string::npos);
    assert(ok.body.find("\"directory\":\"News~Tagesschau\"") != std::string::npos);
    assert(ok.body.find(R"json("title":"Tagesschau","directory":"News~Tagesschau")json") != std::string::npos);
    assert(ok.body.find(R"json("endTime":"2030","priority":68)json") != std::string::npos);
    assert(ok.body.find("\"priority\":68") != std::string::npos);
    assert(ok.body.find("\"lifetime\":19") != std::string::npos);
    assert(ok.body.find(R"json("lifetime":19,"enabled":false,"vps":true)json") != std::string::npos);
    assert(ok.body.find("\"vps\":true") != std::string::npos);
    assert(ok.body.find("\"eventId\":\"117721\"") != std::string::npos);
    assert(ok.body.find("\"flags\":5") != std::string::npos);
    assert(ok.body.find("\"aux\"") == std::string::npos);
    const std::string auxUri =
        "/api/v1/native-timers/17/aux?backendId=backend-one";
    const auto aux = get(auxUri);
    assert(aux.statusCode == 200);
    assert(aux.headers.at("Cache-Control") == "no-store");
    assert(aux.headers.count("ETag") == 0U);
    assert(aux.body.find(
        R"json("aux":"<epgsearch note=\"quoted\">original</epgsearch>")json")
        != std::string::npos);
    assert(get(auxUri, "\"old\"").statusCode == 400);
    assert(get(auxUri + "&limit=1").statusCode == 400);
    assert(get(auxUri, "", "").statusCode == 401);
    assert(ok.body.find("resourceRevision") == std::string::npos);
    assert(ok.headers.at("Cache-Control") == "no-store");
    assert(ok.headers.at("X-Request-ID") == "request");
    assert(ok.headers.at("X-Correlation-ID") == "correlation");
    const std::string tag = ok.headers.at("ETag");
    assert(tag == vdrsuite::http::publicStrongEntityTag(
        "native-timer-binding/1:bind:3:8"));
    const auto cached = get(uri, tag);
    assert(cached.statusCode == 304 && cached.body.empty());
    // Aux may change while the Binding revision remains identical.
    currentAux = "<tv>changed</tv>";
    assert(get(uri, tag).statusCode == 304);
    assert(get(auxUri).body.find("<tv>changed</tv>") != std::string::npos);
    assert(cached.headers.at("ETag") == tag);
    assert(cached.headers.at("Cache-Control") == "no-store");
    assert(cached.headers.at("X-Request-ID") == "request");
    assert(cached.headers.at("X-Correlation-ID") == "correlation");
    assert(get(uri, "invalid").statusCode == 400);
    assert(get(uri, "", "").statusCode == 401);
    assert(get(uri, "", "viewer", "").statusCode == 400);
    assert(get(uri + "&limit=1").statusCode == 400);
    assert(get("/api/v1/native-timers/17?backendId=other").statusCode == 400);
    status = PublicNativeTimerDetailStatus::notFound;
    assert(get(uri).statusCode == 404);
    assert(get(auxUri).statusCode == 404);
    status = PublicNativeTimerDetailStatus::unavailable;
    assert(get(uri).statusCode == 503);
    assert(get(auxUri).statusCode == 503);
    status = PublicNativeTimerDetailStatus::ok;
    bound = false;
    const auto unbound = get(uri);
    assert(unbound.statusCode == 200);
    assert(unbound.headers.count("ETag") == 0U);
    assert(unbound.body.find("binding_required") != std::string::npos);
    assert(unbound.body.find("directActions") == std::string::npos);
    api.registerNativeTimerDirectAction(
        [](const std::string&, const std::string&,
           const std::string&, const std::string&, const std::string&) {
            return ApiResponse{};
        });
    const auto directUnbound = get(uri);
    assert(directUnbound.body.find(R"json("directActions":true)json")
        != std::string::npos);
    bound = true;
    assert(get(uri).body.find("directActions") == std::string::npos);
    bound = false;
    api.resetNativeTimerDirectAction();
    assert(get(uri, tag).statusCode == 400);
    const std::string conflictUri = "/api/v1/native-timers/conflicts/live?backendId=backend-one";
    assert(get(conflictUri).statusCode == 503);
    int reads = 0;
    api.registerNativeTimerDirectAction(
        [&](const std::string& action, const std::string& actor, const std::string& backend,
            const std::string& nativeId, const std::string& body) {
            ++reads;
            assert(action == "conflicts" && actor == "viewer" && backend == "backend-one");
            assert(nativeId.empty() && body.empty());
            ApiResponse response;
            response.statusCode = 200;
            response.contentType = "application/json";
            response.body = R"json({"available":true,"count":1,"total":1,"conflicts":[{"conflictTime":1795550400}]})json";
            return response;
        });
    assert(get(conflictUri,"","").statusCode == 401);
    assert(get(conflictUri,"","viewer","foreign").statusCode == 400);
    assert(get(conflictUri+"&limit=1").statusCode == 400);
    assert(reads == 0);
    const auto conflicts = get(conflictUri);
    assert(conflicts.statusCode == 200 && reads == 1);
    assert(conflicts.body.find(R"json("count":1)json") != std::string::npos);
    assert(conflicts.headers.at("Cache-Control") == "no-store");
    api.resetNativeTimerDirectAction();
    const std::string liveUri = "/api/v1/native-timers/live/list?backendId=backend-one";
    assert(get(liveUri).statusCode == 503);
    int liveReads = 0;
    api.registerNativeTimerDirectAction(
        [&](const std::string& action, const std::string& actor, const std::string& backend,
            const std::string& nativeId, const std::string& body) {
            ++liveReads;
            assert(action == "list" && actor == "viewer" && backend == "backend-one");
            assert(nativeId.empty() && body.empty());
            ApiResponse response;
            response.statusCode = 200;
            response.contentType = "application/json";
            response.body = R"json({"timers":[{"timerId":"9","subtitle":"News","aux":"<epgsearch/>"},{"timerId":"2"}]})json";
            return response;
        });
    assert(get(liveUri,"","").statusCode == 401);
    assert(get(liveUri,"","viewer","foreign").statusCode == 400);
    assert(get(liveUri+"&limit=1").statusCode == 400);
    assert(liveReads == 0);
    const auto live = get(liveUri);
    assert(live.statusCode == 200 && liveReads == 1);
    assert(live.headers.at("Cache-Control") == "no-store");
    assert(live.body.find(R"json("timerId":"9")json") < live.body.find(R"json("timerId":"2")json"));
    assert(live.body.find("<epgsearch/>") != std::string::npos);
    api.resetNativeTimerDirectAction();
    ApiResponse denied;
    assert(api.tryHandleUnsupportedMethod("DELETE", uri, "req", "corr", denied));
    assert(denied.statusCode == 405);
    assert(api.tryHandleDelete(uri, "viewer", "req", "corr", "", "",
        "backend-one", denied));
    assert(denied.statusCode == 428);
}
