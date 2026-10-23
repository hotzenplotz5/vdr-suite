#include "MediaShrinkPlanApiRuntime.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

constexpr const char* rec = "rec_0123456789abcdef0123456789abcdef";
constexpr const char* review = "shk_abcdef0123456789abcdef0123456789";

int main() {
    auto& api = MediaShrinkPlanApiRuntime::instance();
    api.registerMutationHandlers({}, {});
    const std::string base = "/api/v1/backends/backend1";
    const std::string stage = std::string(R"({"recordingId":")") + rec +
        R"(","encoder":"software","maxHeight":720,"nightMode":true})";
    const std::string publish = std::string(R"({"reviewId":")") + review +
        R"(","revision":1})";
    auto post = [&](std::string url, std::string body,
                    std::string actor = "actor1",
                    std::string backend = "backend1",
                    std::string key = "key-12345678") {
        ApiResponse result;
        assert(api.tryHandlePost(url, body, actor, backend,
                                 "application/json", result, key));
        return result;
    };
    const auto stageUrl = base + "/media-shrink-stages";
    const auto publishUrl = base + "/media-shrink-publications";
    assert(post(stageUrl, stage).statusCode == 503);
    assert(post(publishUrl, publish).statusCode == 503);
    assert(post(stageUrl, stage, "").statusCode == 401);
    assert(post(stageUrl, stage, "actor1", "outside").statusCode == 403);
    assert(post(stageUrl, stage, "actor1", "backend1", "bad").statusCode == 400);
    assert(post(stageUrl + "?path=/tmp/evil", stage).statusCode == 400);
    assert(post(stageUrl, stage.substr(0, stage.size()-1) +
                R"(,"path":"/tmp/evil"})").statusCode == 422);
    assert(post(publishUrl, R"({"reviewId":"../../evil","revision":1})")
               .statusCode == 422);
    assert(post(publishUrl, std::string(R"({"reviewId":")") + review +
                R"(","revision":2})").statusCode == 422);
    int staged = 0, published = 0;
    api.registerMutationHandlers(
        [&](const std::string& actor, const std::string& backend,
            const std::string& recording, const std::string& encoder,
            unsigned maxHeight, bool nightMode, const std::string& key) {
            assert(actor == "actor1" && backend == "backend1" &&
                   recording == rec && encoder == "software" &&
                   maxHeight == 720 && nightMode && key == "key-12345678");
            ++staged;
            return MediaShrinkPlanApiRuntime::json(202, R"({"accepted":true})");
        },
        [&](const std::string& actor, const std::string& backend,
            const std::string& id, std::uint64_t revision,
            const std::string& key) {
            assert(actor == "actor1" && backend == "backend1" &&
                   id == review && revision == 1 && key == "key-12345678");
            ++published;
            return MediaShrinkPlanApiRuntime::json(202, R"({"accepted":true})");
        });
    assert(post(stageUrl, stage).statusCode == 202);
    assert(post(publishUrl, publish).statusCode == 202);
    assert(staged == 1 && published == 1);
    assert(post(stageUrl, stage, "actor1", "outside").statusCode == 403);
    assert(staged == 1 && published == 1);
    api.registerMutationHandlers({}, {});
    const std::string operation = "op_0123456789abcdef0123456789abcdef";
    const std::string operationUrl =
        base + "/media-shrink-operations/" + operation;
    auto get = [&](const std::string& url,
                   const std::string& actor = "actor1",
                   const std::string& backend = "backend1") {
        ApiResponse result;
        assert(api.tryHandleGet(url, actor, backend, {}, result));
        return result;
    };
    assert(get(operationUrl).statusCode == 503);
    assert(get(operationUrl, "").statusCode == 401);
    assert(get(operationUrl, "actor1", "outside").statusCode == 404);
    assert(get(operationUrl + "?unsafe=/etc/passwd").statusCode == 400);
    assert(get(base + "/media-shrink-operations/not-an-id").statusCode == 404);
    int reads = 0;
    api.registerHandlers({}, {}, {},
        [&](const std::string& actor, const std::string& backend,
            const std::string& id) {
            assert(actor == "actor1" && backend == "backend1" &&
                   id == operation);
            ++reads;
            return MediaShrinkPlanApiRuntime::json(
                200, R"({"action":"stage","state":"dispatching"})");
        });
    assert(get(operationUrl).statusCode == 200);
    assert(reads == 1);
    assert(get(operationUrl, "actor1", "outside").statusCode == 404);
    assert(reads == 1);
    api.registerHandlers({}, {});
    std::cout << "MEDIA_SHRINK_V1_MUTATION_API=PASS "
              << "BACKEND_SCOPE=PASS OPERATION_READ=PASS FAIL_CLOSED=YES\n";
}
