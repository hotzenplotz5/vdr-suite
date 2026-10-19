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
    auto status = PublicNativeTimerDetailStatus::ok;
    api.registerNativeTimerDetailLookup([&](const PublicNativeTimerDetailRequest& req)
    {
        assert(req.backendId == "backend-one" && req.timerId == "17");
        PublicNativeTimerDetailResult r;
        r.status = status;
        r.backendId = req.backendId;
        r.timer.id = req.timerId;
        r.timer.title = "Tagesschau";
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
    assert(ok.body.find("resourceRevision") == std::string::npos);
    const std::string tag = ok.headers.at("ETag");
    assert(tag == vdrsuite::http::publicStrongEntityTag(
        "native-timer-binding/1:bind:3:8"));
    const auto cached = get(uri, tag);
    assert(cached.statusCode == 304 && cached.body.empty());
    assert(get(uri, "invalid").statusCode == 400);
    assert(get(uri, "", "").statusCode == 401);
    assert(get(uri, "", "viewer", "").statusCode == 400);
    assert(get(uri + "&limit=1").statusCode == 400);
    assert(get("/api/v1/native-timers/17?backendId=other").statusCode == 400);
    status = PublicNativeTimerDetailStatus::notFound;
    assert(get(uri).statusCode == 404);
    status = PublicNativeTimerDetailStatus::unavailable;
    assert(get(uri).statusCode == 503);
    status = PublicNativeTimerDetailStatus::ok;
    bound = false;
    const auto unbound = get(uri);
    assert(unbound.statusCode == 200);
    assert(unbound.headers.count("ETag") == 0U);
    assert(unbound.body.find("binding_required") != std::string::npos);
    assert(get(uri, tag).statusCode == 400);
    ApiResponse denied;
    assert(api.tryHandleUnsupportedMethod("DELETE", uri, "req", "corr", denied));
    assert(denied.statusCode == 405);
    assert(api.tryHandleDelete(uri, "viewer", "req", "corr", "", "",
        "backend-one", denied));
    assert(denied.statusCode == 428);
}
