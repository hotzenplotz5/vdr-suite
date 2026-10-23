#include "DaemonMediaShrinkPlanRuntime.h"
#include "MediaShrinkPlanApiRuntime.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
constexpr const char* kRecordingId = "rec_0123456789abcdef0123456789abcdef";

int main() {
    const auto sample = (fs::temp_directory_path() /
        "vdr-suite-shrink-plan-XXXXXX").string();
    std::vector<char> buffer(sample.begin(), sample.end());
    buffer.push_back('\0');
    const char* created = ::mkdtemp(buffer.data());
    assert(created);
    const fs::path home(created);
    struct Cleanup {
        fs::path home;
        ~Cleanup() { std::error_code e; fs::remove_all(home, e); }
    } cleanup{home};
    const fs::path root = home / "video";
    const fs::path recording = root / "Example" /
        "2026-10-22.12.00.1-0.rec";
    fs::create_directories(recording);
    std::ofstream(recording / "00001.ts", std::ios::binary)
        << std::string(32768, 'X');
    std::ofstream(recording / "info") << "T Example\n";
    std::ofstream(recording / "index") << "native-index";
    const std::set<std::string> originalNames = {
        "00001.ts", "info", "index"
    };
    std::string before;
    {
        std::ifstream in(recording / "00001.ts", std::ios::binary);
        before.assign(std::istreambuf_iterator<char>(in),
                      std::istreambuf_iterator<char>());
    }
    DaemonMediaShrinkPlanRuntime runtime(
        {{"local", root.string()}},
        [&](const std::string& backend, const std::string& id)
            -> std::optional<fs::path> {
            if (backend == "local" && id == kRecordingId)
                return recording;
            return std::nullopt;
        },
        [](const std::string& backend) {return backend == "local";});
    auto& api = MediaShrinkPlanApiRuntime::instance();
    api.registerHandlers(
        [&](const std::string& actor, const std::string& backend,
            const std::string& id) {
            return runtime.plan(actor, backend, id);
        },
        [&](const std::string& backend) {
            return runtime.capability(backend);
        });
    auto post = [&](const std::string& body, const std::string& actor,
                    const std::string& scope) {
        ApiResponse response;
        assert(api.tryHandlePost(
            "/api/v1/backends/local/media-shrink-plans",
            body, actor, scope, "application/json", response));
        return response;
    };
    const std::string request = std::string("{\"recordingId\":\"") +
        kRecordingId + "\"}";
    assert(post(request, "", "local").statusCode == 401);
    assert(post(request, "actor-a", "elsewhere").statusCode == 403);
    assert(post("{\"recordingId\":\"../../etc/passwd\"}",
                "actor-a", "local").statusCode == 422);
    assert(post(request.substr(0, request.size() - 1) +
                ",\"path\":\"/tmp/evil\"}", "actor-a", "local").statusCode == 422);
    assert(post("{\"recordingId\":\"rec_ffffffffffffffffffffffffffffffff\"}",
                "actor-a", "local").statusCode == 404);
    ApiResponse capability;
    assert(api.tryHandleGet(
        "/api/v1/backends/local/capabilities/media-shrink",
        "actor-a", "local", {}, capability));
    assert(capability.statusCode == 200);
    assert(capability.body.find("\"planningAvailable\":true") !=
           std::string::npos);
    assert(capability.body.find("\"available\":false") !=
           std::string::npos);
    assert(api.tryHandleGet(
        "/api/v1/backends/local/capabilities/media-shrink",
        "actor-a", "other", {}, capability));
    assert(capability.statusCode == 404);
    const auto reply = post(request, "actor-a", "local");
    assert(reply.statusCode == 200);
    assert(reply.body.find("\"readOnly\":true") != std::string::npos);
    assert(reply.body.find("\"stageAvailable\":false") != std::string::npos);
    assert(reply.body.find("\"sourceBytes\":32768") != std::string::npos);
    assert(reply.body.find("\"segments\":1") != std::string::npos);
    assert(reply.body.find(home.string()) == std::string::npos);
    assert(reply.body.find("2026-10-22.12.00.1-0.rec") == std::string::npos);

    std::set<std::string> names;
    for (const auto& item : fs::directory_iterator(recording))
        names.insert(item.path().filename().string());
    assert(names == originalNames);
    std::ifstream in(recording / "00001.ts", std::ios::binary);
    const std::string after{std::istreambuf_iterator<char>(in),
                            std::istreambuf_iterator<char>()};
    assert(before == after);
    // Failed-open if a recording path is replaced by a symlink.
    fs::rename(recording / "00001.ts", recording / "saved.ts");
    fs::create_symlink(recording / "saved.ts", recording / "00001.ts");
    assert(post(request, "actor-a", "local").statusCode == 422);
    api.registerHandlers({}, {});
}
