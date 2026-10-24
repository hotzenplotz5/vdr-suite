#include "PublicApiRuntime.h"

#include <cassert>
#include <string>

int main()
{
    auto& api = PublicApiRuntime::instance();
    api.resetMediaImportAdmission();
    const std::string path =
        "/api/v1/backends/backend-a/media-imports";
    const std::string body =
        "{\"sourceId\":\"opaque_source_123456\"}";

    auto request = [&](const std::string& input,
                       const std::string& actor,
                       const std::string& backend,
                       const std::string& key)
    {
        ApiResponse response;
        assert(api.tryHandlePost(path, "a7-post", "a7-corr", response,
            input, actor, "", key, "application/json", backend));
        return response;
    };
    assert(request(body, "actor:alice", "backend-a",
                   "a7-idem").statusCode == 503);

    unsigned int admissions = 0;
    std::string expectedDecision;
    api.registerMediaImportAdmission(
        [&](const PublicMediaImportAdmissionRequest& input)
        {
            ++admissions;
            assert(input.actorRef == "actor:alice");
            assert(input.backendId == "backend-a");
            assert(input.sourceId == "opaque_source_123456");
            assert(input.idempotencyKey == "a7-idem");
            assert(input.videoDecision == expectedDecision);
            assert(input.approveVideoReencode == (expectedDecision == "local"));
            PublicMediaImportAdmissionResult r;
            r.status = PublicMediaImportAdmissionStatus::accepted;
            r.operation.operationId = "op_12345678901234567890123456789012";
            r.operation.backendId = input.backendId;
            r.operation.state = "dispatching";
            r.operation.resourceRevision = "2";
            return r;
        });

    const ApiResponse accepted =
        request(body, "actor:alice", "backend-a", "a7-idem");
    assert(accepted.statusCode == 202);
    assert(accepted.headers.at("Location") ==
        "/api/v1/operations/op_12345678901234567890123456789012");
    assert(accepted.body.find("\"state\":\"dispatching\"") !=
        std::string::npos);
    assert(admissions == 1);

    // Authenticated Public API path: only explicit known decisions.
    for (const char* choice : {"local", "desktop", "skip"}) {
        expectedDecision = choice;
        const std::string payload =
            std::string("{\"sourceId\":\"opaque_source_123456\",") +
            "\"videoDecision\":\"" + choice + "\"}";
        assert(request(payload, "actor:alice", "backend-a",
                       "a7-idem").statusCode == 202);
    }
    expectedDecision.clear();
    for (const char* choice : {"unknown", "pc_encode", "../escape", ""}) {
        const std::string payload =
            std::string("{\"sourceId\":\"opaque_source_123456\",") +
            "\"videoDecision\":\"" + choice + "\"}";
        assert(request(payload, "actor:alice", "backend-a",
                       "a7-idem").statusCode == 422);
    }
    assert(admissions == 4);

    assert(request(body, "", "backend-a", "a7-idem").statusCode == 401);
    assert(request(body, "actor:alice", "backend-b",
                   "a7-idem").statusCode == 404);
    assert(request("{\"sourceId\":\"../../etc/passwd\"}", "actor:alice",
                   "backend-a", "a7-idem").statusCode == 422);
    assert(request("{\"sourceId\":\"opaque_source_123456\",\"path\":\"/tmp\"}",
                   "actor:alice", "backend-a", "a7-idem").statusCode == 422);
    assert(request("{\"sourceId\":\"opaque_source_123456\",\"sourceId\":\"opaque_source_123456\"}",
                   "actor:alice", "backend-a", "a7-idem").statusCode == 422);
    assert(request(body, "actor:alice", "backend-a", "").statusCode == 400);
    assert(admissions == 4);

    ApiResponse get;
    assert(api.tryHandleGet(path, "actor:alice", "", "", get));
    assert(get.statusCode == 405);
    assert(get.headers.at("Allow") == "POST");
    ApiResponse patch;
    assert(api.tryHandleUnsupportedMethod(
        "PATCH", path, "", "", patch));
    assert(patch.statusCode == 405);
    assert(patch.headers.at("Allow") == "POST");
    api.resetMediaImportAdmission();
    return 0;
}
