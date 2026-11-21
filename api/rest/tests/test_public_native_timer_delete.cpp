#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"

#include <cassert>
#include <string>

int main()
{
    auto& api = PublicApiRuntime::instance();
    api.resetNativeTimerDeleteAdmission();
    const std::string uri = "/api/v1/native-timers/17?backendId=backend-one";
    const auto etag = vdrsuite::http::publicStrongEntityTag(
        "native-timer-binding/1:bind17:3:8");
    auto call = [&](const std::string& target, const std::string& match,
                    const std::string& key, const std::string& backend = "backend-one")
    {
        ApiResponse r;
        assert(api.tryHandleDelete(target, "actor:tv", "request", "correlation",
            match, key, backend, r));
        return r;
    };
    assert(call(uri, "", "delete-key").statusCode == 428);
    assert(call(uri, "W/" + etag, "delete-key").statusCode == 400);
    assert(call(uri, etag, "").statusCode == 400);
    assert(call(uri + "&limit=1", etag, "delete-key").statusCode == 400);
    assert(call(uri, etag, "delete-key").statusCode == 503);
    int count = 0;
    PublicNativeTimerDeleteStatus status = PublicNativeTimerDeleteStatus::accepted;
    api.registerNativeTimerDeleteAdmission([&](const PublicNativeTimerDeleteRequest& req)
    {
        ++count;
        assert(req.backendId == "backend-one");
        assert(req.timerId == "17");
        assert(req.expectedResourceRevision ==
            "native-timer-binding/1:bind17:3:8");
        assert(req.idempotencyKey == "delete-key");
        PublicNativeTimerDeleteResult r;
        r.status = status;
        r.operation.operationId = "op_delete_1";
        r.operation.state = "accepted";
        r.operation.backendId = "backend-one";
        r.operation.resourceRevision = "1";
        return r;
    });
    const auto ok = call(uri, etag, "delete-key");
    assert(ok.statusCode == 202);
    assert(ok.headers.at("Location") == "/api/v1/operations/op_delete_1");
    assert(count == 1);
    status = PublicNativeTimerDeleteStatus::replayed;
    assert(call(uri, etag, "delete-key").statusCode == 202);
    status = PublicNativeTimerDeleteStatus::adoptionRequired;
    assert(call(uri, etag, "delete-key").statusCode == 409);
    status = PublicNativeTimerDeleteStatus::revisionConflict;
    assert(call(uri, etag, "delete-key").statusCode == 412);
    status = PublicNativeTimerDeleteStatus::notFound;
    assert(call(uri, etag, "delete-key").statusCode == 404);
    status = PublicNativeTimerDeleteStatus::backendUnavailable;
    assert(call(uri, etag, "delete-key").statusCode == 503);
    api.resetNativeTimerDeleteAdmission();
}
