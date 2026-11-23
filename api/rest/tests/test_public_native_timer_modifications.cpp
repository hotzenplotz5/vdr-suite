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
            assert(r.kind == PublicNativeTimerChangeKind::toggle);
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

    // Raw Aux replacement is an opt-in extension of the existing native
    // update route, using the caller's previous Aux for native CAS.
    const std::string update =
        R"json({"kind":"update","channelId":"S19.2E-1-2-3","title":"News","directory":"TV","day":"2026-11-24","weekdays":"-------","startTime":"2015","endTime":"2030","priority":0,"lifetime":0,"enabled":true,"vps":false})json";
    const std::string auxUpdate = update.substr(0,update.size()-1) +
        R"json(,"expectedAux":"<epgsearch/>","aux":"<tvscraper/>"})json";
    assert(post(path,auxUpdate,etag).statusCode == 503);
    assert(post(path,update.substr(0,update.size()-1) +
        R"json(,"aux":"bad"})json",etag).statusCode == 400);
    assert(post(path,update.substr(0,update.size()-1) +
        R"json(,"expectedAux":"bad"})json",etag).statusCode == 400);
    assert(post(path,toggle.substr(0,toggle.size()-1) +
        R"json(,"expectedAux":"","aux":"new"})json",etag).statusCode == 400);
    int auxAdmissions = 0;
    runtime.registerNativeTimerModifyAdmission(
        [&](const PublicNativeTimerModifyRequest& r) {
            ++auxAdmissions;
            assert(r.kind == PublicNativeTimerChangeKind::update);
            assert(r.specification.modifyAux);
            assert(r.specification.expectedAux == "<epgsearch/>");
            assert(r.specification.aux == "<tvscraper/>");
            assert(r.specification.priority == 0 && r.specification.lifetime == 0);
            PublicNativeTimerModifyResult result;
            result.status = PublicNativeTimerModifyStatus::accepted;
            result.operation.operationId = "op_modify_aux";
            result.operation.state = "accepted";
            result.operation.backendId = "backend-one";
            result.operation.resourceRevision = "2";
            return result;
        });
    assert(post(path,auxUpdate,etag).statusCode == 202);
    assert(auxAdmissions == 1);
    runtime.resetNativeTimerModifyAdmission();

    // Slice 3: externally created and unbound native VDR Timers use exactly
    // one existing direct action callback, never a manufactured 202 Operation.
    runtime.resetNativeTimerDirectAction();
    runtime.resetNativeTimerDetailLookup();
    const std::string directUpdate =
        "/api/v1/native-timers/17/direct-update?backendId=backend-one";
    const std::string directDelete =
        "/api/v1/native-timers/17/direct-delete?backendId=backend-one";
    const std::string directBody =
        R"json({"backendId":"backend-one","timerId":"17","title":"AUX","aux":"<epgsearch/>"})json";
    assert(post(directUpdate,directBody).statusCode == 503);
    std::string bindingState = "binding_required";
    std::string ownership = "unbound";
    runtime.registerNativeTimerDetailLookup(
        [&](const PublicNativeTimerDetailRequest& req) {
            PublicNativeTimerDetailResult result;
            result.status = PublicNativeTimerDetailStatus::ok;
            result.backendId = req.backendId;
            result.timer.id = req.timerId;
            result.ownership = ownership;
            result.bindingState = bindingState;
            return result;
        });
    int directCalls = 0;
    runtime.registerNativeTimerDirectAction(
        [&](const std::string& action, const std::string& actor,
            const std::string& backend, const std::string& nativeId,
            const std::string& raw) {
            ++directCalls;
            assert((action == "update" || action == "delete"));
            assert(actor == "tv-device");
            assert(backend == "backend-one" && nativeId == "17");
            assert(raw == directBody);
            ApiResponse result;
            result.statusCode = 200;
            result.contentType = "application/json";
            result.body = R"json({"success":true})json";
            return result;
        });
    assert(post(directUpdate,directBody).statusCode == 200);
    assert(directCalls == 1);
    assert(post(directDelete,directBody).statusCode == 200);
    assert(directCalls == 2);
    ownership = "external";
    bindingState = "adoption_required";
    assert(post(directUpdate,directBody).statusCode == 200);
    assert(directCalls == 3);
    ownership = "managed";
    bindingState = "verified";
    assert(post(directUpdate,directBody).statusCode == 409);
    assert(directCalls == 3);
    ownership = "unbound";
    bindingState = "binding_required";
    assert(post(directUpdate+"&limit=1",directBody).statusCode == 400);
    assert(post(directUpdate,directBody,"","","text/plain").statusCode == 415);
    assert(post(directUpdate,directBody,"","", "application/json").statusCode == 200);
    runtime.resetNativeTimerDirectAction();
    runtime.resetNativeTimerDetailLookup();
}
