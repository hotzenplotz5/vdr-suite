#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"

#include <cassert>
#include <string>

int main()
{
    auto& runtime = PublicApiRuntime::instance();
    runtime.resetNativeTimerModifyAdmission();
    const std::string path = "/api/v1/native-timers/17/modifications?backendId=backend-one";
    const auto etag = vdrsuite::http::publicStrongEntityTag(
        "native-timer-binding/1:bind17:7:10");
    auto post = [&](const std::string& target, const std::string& body,
                    const std::string& revision = "",
                    const std::string& key = "modify-key",
                    const std::string& contentType = "application/json")
    {
        ApiResponse response;
        assert(runtime.tryHandlePost(
            target, "request", "correlation", response, body, "tv-device",
            revision, key, contentType, "backend-one", ""));
        return response;
    };
    const std::string toggle = R"({"kind":"toggle","enabled":false})";
    assert(post(path,toggle).statusCode == 428);
    assert(post(path,toggle,"W/"+etag).statusCode == 400);
    assert(post(path,toggle,etag,"").statusCode == 400);
    assert(post(path,toggle,etag,"modify-key","text/plain").statusCode == 415);
    assert(post(path+"x",toggle,etag).statusCode != 202);
    assert(post(path+"&limit=1",toggle,etag).statusCode == 400);
    assert(post(path,R"({"kind":"toggle","enabled":true,"title":"bad"})",etag).statusCode == 400);
    assert(post(path,R"({"kind":"toggle","enabled":true,"enabled":false})",etag).statusCode == 400);
    assert(post(path,R"({"kind":"update","enabled":true})",etag).statusCode == 400);
    assert(post(path,toggle,etag).statusCode == 503);
    int called = 0;
    PublicNativeTimerModifyStatus status = PublicNativeTimerModifyStatus::accepted;
    runtime.registerNativeTimerModifyAdmission(
        [&](const PublicNativeTimerModifyRequest& r)
        {
            ++called;
            assert(r.actorRef == "tv-device");
            assert(r.backendId == "backend-one" && r.timerId == "17");
            assert(r.expectedResourceRevision == "native-timer-binding/1:bind17:7:10");
            assert(r.idempotencyKey == "modify-key");
            assert(r.kind == vdrsuite::timers::NativeTimerModifyKind::toggle);
            assert(!r.specification.enabled);
            PublicNativeTimerModifyResult result;
            result.status = status;
            result.operation.operationId = "op_modify_1";
            result.operation.state = "accepted";
            result.operation.backendId = "backend-one";
            result.operation.resourceRevision = "1";
            return result;
        });
    auto accepted = post(path,toggle,etag);
    assert(accepted.statusCode == 202);
    assert(accepted.headers.at("Location") == "/api/v1/operations/op_modify_1");
    assert(called == 1);
    status = PublicNativeTimerModifyStatus::replayed;
    assert(post(path,toggle,etag).statusCode == 202);
    status = PublicNativeTimerModifyStatus::adoptionRequired;
    assert(post(path,toggle,etag).statusCode == 409);
    status = PublicNativeTimerModifyStatus::revisionConflict;
    assert(post(path,toggle,etag).statusCode == 412);
    status = PublicNativeTimerModifyStatus::notFound;
    assert(post(path,toggle,etag).statusCode == 404);
    status = PublicNativeTimerModifyStatus::serviceUnavailable;
    assert(post(path,toggle,etag).statusCode == 503);
    runtime.resetNativeTimerModifyAdmission();
}
