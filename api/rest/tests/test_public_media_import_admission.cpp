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
    std::string expectedMetadata;
    api.registerMediaImportAdmission(
        [&](const PublicMediaImportAdmissionRequest& input)
        {
            ++admissions;
            assert(input.actorRef == "actor:alice");
            assert(input.backendId == "backend-a");
            assert(input.sourceId == "opaque_source_123456");
            assert(input.idempotencyKey == "a7-idem");
            assert(input.videoDecision ==
                   (expectedDecision == "local" ? "" : expectedDecision));
            assert(input.approveVideoReencode == (expectedDecision == "local"));
            assert(input.metadata == expectedMetadata);
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
    for (const char* choice : {"local", "desktop", "skip", "release", "duplicate", "delete_duplicate"}) {
        expectedDecision = choice;
        const std::string payload =
            std::string("{\"sourceId\":\"opaque_source_123456\",") +
            "\"videoDecision\":\"" + choice + "\"}";
        assert(request(payload, "actor:alice", "backend-a",
                       "a7-idem").statusCode == 202);
    }
    expectedDecision.clear();
    // The pre-selected TMDb episode is frozen in the same import POST.
    // v1 remains compatible; v2 adds the provider identity and artwork.
    const std::string snapshotV1 =
        "v1|episode|Andor|Das%20Auge|Episode%206|1|6";
    const std::string snapshotV2 =
        "v2|episode|Andor|Das%20Auge|Episode%206|1|6|tv-episode|123456|";
    for (const std::string& snapshot : {snapshotV1, snapshotV2}) {
        expectedMetadata = snapshot;
        const auto payload = std::string("{\"sourceId\":\"opaque_source_123456\",") +
            "\"metadata\":\"" + snapshot + "\"}";
        assert(request(payload, "actor:alice", "backend-a",
                       "a7-idem").statusCode == 202);
    }
    expectedMetadata.clear();
    for (const std::string& snapshot : {
        std::string("v2|episode|Andor|Das%20Auge|Episode%206|1|6"),
        std::string("v3|episode|Andor|Das%20Auge|Episode%206|1|6|tv-episode|123456|")
    }) {
        const auto payload = std::string("{\"sourceId\":\"opaque_source_123456\",") +
            "\"metadata\":\"" + snapshot + "\"}";
        assert(request(payload, "actor:alice", "backend-a",
                       "a7-idem").statusCode == 422);
    }
    for (const char* choice : {"unknown", "pc_encode", "../escape", ""}) {
        const std::string payload =
            std::string("{\"sourceId\":\"opaque_source_123456\",") +
            "\"videoDecision\":\"" + choice + "\"}";
        assert(request(payload, "actor:alice", "backend-a",
                       "a7-idem").statusCode == 422);
    }
    assert(admissions == 9);

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
    assert(admissions == 9);

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
