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
            if (!input.batchSources.empty()) {
                const std::string batchId =
                    "mb1_0123456789abcdef0123456789abcdef";
                assert(input.idempotencyKey == batchId);
                assert(input.batchSources.size() == 2U);
                assert(input.batchSources[0].sourceId ==
                       "opaque_source_123456");
                assert(input.batchSources[0].metadata ==
                       "v1|episode|Andor|Auge|Episode%206|1|6");
                assert(input.batchSources[1].sourceId ==
                       "opaque_source_654321");
                assert(input.batchSources[1].metadata.empty());
                PublicMediaImportAdmissionResult r;
                r.status = PublicMediaImportAdmissionStatus::accepted;
                r.batchId = batchId;
                for (std::size_t index = 0; index < 2U; ++index) {
                    PublicOperationResource item;
                    item.operationId = index == 0U
                        ? "op_12345678901234567890123456789012"
                        : "op_abcdefabcdefabcdefabcdefabcdefab";
                    item.backendId = "backend-a";
                    item.state = index == 0U ? "dispatching" : "accepted";
                    item.resourceRevision = "1";
                    r.batchOperations.push_back(item);
                    r.batchSourceIds.push_back(
                        input.batchSources[index].sourceId);
                }
                r.operation = r.batchOperations.front();
                return r;
            }
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

    // One client confirmation atomically reserves both canonical source
    // Operations; only the first is dispatching. A browser disconnect does
    // not cause a second POST for the later source.
    const std::string batchKey =
        "mb1_0123456789abcdef0123456789abcdef";
    const std::string batchBody =
        "{\"batch\":[{\"sourceId\":\"opaque_source_123456\","
        "\"metadata\":\"v1|episode|Andor|Auge|Episode%206|1|6\"},"
        "{\"sourceId\":\"opaque_source_654321\"}]}";
    const ApiResponse batch = request(
        batchBody, "actor:alice", "backend-a", batchKey);
    assert(batch.statusCode == 202);
    assert(batch.body.find("\"batchId\":\"" + batchKey + "\"") !=
           std::string::npos);
    assert(batch.body.find("\"batchOperations\":[") !=
           std::string::npos);
    assert(batch.body.find("opaque_source_654321") != std::string::npos);
    assert(batch.body.find("op_abcdefabcdefabcdefabcdefabcdefab") !=
           std::string::npos);
    assert(admissions == 10);
    assert(request("{\"batch\":[{\"sourceId\":\"opaque_source_123456\"}]}",
                   "actor:alice", "backend-a", batchKey).statusCode == 422);
    assert(request(batchBody, "actor:alice", "backend-a", "a7-idem")
                   .statusCode == 400);
    assert(request("{\"batch\":[{\"sourceId\":\"opaque_source_123456\","
                   "\"videoDecision\":\"desktop\"},{\"sourceId\":"
                   "\"opaque_source_654321\"}]}",
                   "actor:alice", "backend-a", batchKey).statusCode == 422);
    assert(admissions == 10);

    ApiResponse get;
    assert(api.tryHandleGet(path, "actor:alice", "", "", get));
    // An actor without backend read scope cannot enumerate imports.
    assert(get.statusCode == 404);
    ApiResponse scopedGet;
    assert(api.tryHandleGet(path, "actor:alice", "", "", scopedGet,
                            "", "backend-a"));
    assert(scopedGet.statusCode == 503);
    // The new read route reports unavailable until its Core lookup is bound.
    api.registerMediaImportRecentLookup(
        [](const std::string& actor, const std::string& backend) {
            PublicMediaImportRecentResult out;
            if (actor == "actor:alice" && backend == "backend-a")
                out.available = true;
            return out;
        });
    ApiResponse ownedGet;
    assert(api.tryHandleGet(path, "actor:alice", "", "", ownedGet,
                            "", "backend-a"));
    assert(ownedGet.statusCode == 200);
    assert(ownedGet.body.find("\"items\":[]") != std::string::npos);
    api.resetMediaImportRecentLookup();
    ApiResponse patch;
    assert(api.tryHandleUnsupportedMethod(
        "PATCH", path, "", "", patch));
    assert(patch.statusCode == 405);
    assert(patch.headers.at("Allow") == "POST");
    api.resetMediaImportAdmission();
    return 0;
}
