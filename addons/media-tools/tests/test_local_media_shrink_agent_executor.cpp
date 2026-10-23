#include "LocalMediaShrinkAgentExecutor.h"

#include <algorithm>
#include <cassert>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>

namespace fs = std::filesystem;
using namespace vdrsuite::mediatools;

namespace {
constexpr const char* kRecording = "rec_0123456789abcdef0123456789abcdef";

struct Process final : ProcessRunner {
    unsigned transcodes = 0;
    bool failTranscode = false;
    ProcessResult run(const std::vector<std::string>& args, bool captured) override {
        const auto has = [&](const std::string& s) {
            return std::find(args.begin(), args.end(), s) != args.end();
        };
        if (args.at(0) == "/usr/bin/ffprobe") {
            assert(captured);
            if (has("stream=codec_name")) {
                if (has("a")) return {0, "aac\n"};
                return {0, args.back().find("input.ts") != std::string::npos ?
                    "h264\n" : "hevc\n"};
            }
            if (has("format=duration")) return {0, "60.0\n"};
            if (has("stream=height")) return {0, "1080\n"};
        }
        if (args.at(0) == "/usr/bin/ffmpeg") {
            assert(!captured && has("libx265") && has("-fs"));
            ++transcodes;
            if (failTranscode) return {1, ""};
            std::ofstream(args.back(), std::ios::binary) <<
                std::string(8192, 'H');
            return {0, ""};
        }
        if (args.at(0) == "/usr/bin/vdr") {
            assert(!captured && args.size() == 2);
            const auto prefix = std::string("--genindex=");
            assert(args[1].compare(0, prefix.size(), prefix) == 0);
            const fs::path candidate(args[1].substr(prefix.size()));
            std::ofstream(candidate / "index") << "INDEX";
            return {0, ""};
        }
        return {1, ""};
    }
};

BackendAgentCommandAssignment assignment() {
    BackendAgentCommandAssignment a;
    a.present = true;
    a.operationId = "op_stage-a";
    a.commandId = "cmd_stage-a";
    a.backendId = "backend-a";
    a.backendGeneration = 7;
    a.agentId = "agent-a";
    a.agentInstanceId = "instance-a";
    a.jobId = "job-a";
    a.attemptId = "attempt-a";
    a.claimEpoch = 1;
    a.payloadVersion = 1;
    a.commandType = kMediaShrinkStageCommandType;
    a.payload = std::string("{\"recordingId\":\"") + kRecording +
        "\",\"encoder\":\"software\",\"maxHeight\":0,\"nightMode\":false}";
    a.requestFingerprint = "request-fingerprint-a";
    a.verificationPolicy = "readback_required";
    a.deadline = static_cast<std::int64_t>(std::time(nullptr)) + 3600;
    return a;
}
} // namespace

int main() {
    const std::string pattern =
        (fs::temp_directory_path() / "suite-shrink-agent-test-XXXXXX").string();
    std::vector<char> chars(pattern.begin(), pattern.end());
    chars.push_back('\0');
    char* temporary = ::mkdtemp(chars.data());
    assert(temporary);
    const fs::path home(temporary);
    struct Cleanup {
        fs::path path;
        ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); }
    } cleanup{home};
    const auto root = home / "video";
    const auto recording = root / "Example" / "2026-10-23.09.00.1-0.rec";
    const auto state = home / "agent";
    fs::create_directories(recording);
    fs::create_directory(state);
    assert(::chmod(state.c_str(), 0700) == 0);
    std::ofstream(recording / "00001.ts", std::ios::binary)
        << std::string(32768, 'X');
    std::ofstream(recording / "info") << "T HEVC smoke\n";
    std::ofstream(recording / "index") << "INDEX";
    const auto original = [&] {
        std::ifstream file(recording / "00001.ts", std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(file),
                           std::istreambuf_iterator<char>());
    }();
    const fs::path relative = fs::path("Example") / recording.filename();
    const std::string fingerprint =
        shrinkSourceFingerprint(inspectShrink(root, relative));
    LocalMediaShrinkReviewStore store(state);
    Process process;
    bool incorrectFingerprint = false;
    bool available = true;
    bool allowed = true;
    unsigned authorizeCalls = 0;
    LocalMediaShrinkAgentExecutor executor(
        store,
        [&](const std::string&) { return available; },
        [&](const BackendAgentCommandAssignment& a)
            -> std::optional<LocalMediaShrinkAuthority> {
            ++authorizeCalls;
            if (!allowed) return std::nullopt;
            return LocalMediaShrinkAuthority{
                "actor-a", "backend-a", a.operationId, kRecording,
                incorrectFingerprint ? "forged_snapshot" : fingerprint,
                7, root, relative
            };
        },
        [&]() -> ProcessRunner& { return process; });
    auto stage = assignment();
    assert(executor.available(kMediaShrinkStageCommandType));
    assert(!executor.available("addon.media.shrink.erase"));
    available = false;
    assert(!executor.authorize(stage, "backend-a", 7));
    available = true;
    assert(!executor.authorize(stage, "backend-a", 6));
    auto forged = stage;
    forged.payload += ",\"path\":\"/etc/passwd\"";
    assert(!executor.authorize(forged, "backend-a", 7));
    incorrectFingerprint = true;
    assert(executor.authorize(stage, "backend-a", 7));
    assert(executor.execute(stage) == MediaShrinkAgentOutcome::rejected);
    assert(process.transcodes == 0);
    incorrectFingerprint = false;
    assert(executor.authorize(stage, "backend-a", 7));
    allowed = false;
    assert(executor.execute(stage) == MediaShrinkAgentOutcome::rejected);
    assert(process.transcodes == 0);
    allowed = true;
    assert(executor.authorize(stage, "backend-a", 7));
    auto altered = stage;
    altered.requestFingerprint = "different";
    assert(executor.execute(altered) == MediaShrinkAgentOutcome::rejected);
    assert(executor.execute(stage) == MediaShrinkAgentOutcome::submitted);
    assert(process.transcodes == 1);
    assert(original == [&] {
        std::ifstream file(recording / "00001.ts", std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(file),
                           std::istreambuf_iterator<char>());
    }());
    const auto id = executor.takeReviewId();
    assert(id.size() == 36 && id.compare(0, 4, "shk_") == 0);
    assert(executor.takeReviewId().empty());
    assert(store.find(id));
    assert(executor.execute(stage) == MediaShrinkAgentOutcome::rejected);
    // A new executor process can re-authorize the same assignment but may
    // never transcode it again, even if the assignment's attempt changed.
    LocalMediaShrinkReviewStore reopened(state);
    LocalMediaShrinkAgentExecutor restarted(
        reopened,
        [&](const std::string&) { return available; },
        [&](const BackendAgentCommandAssignment& a)
            -> std::optional<LocalMediaShrinkAuthority> {
            if (!allowed) return std::nullopt;
            return LocalMediaShrinkAuthority{
                "actor-a", "backend-a", a.operationId, kRecording,
                fingerprint, 7, root, relative
            };
        },
        [&]() -> ProcessRunner& { return process; });
    auto replay = stage;
    replay.attemptId = "attempt-after-restart";
    assert(restarted.authorize(replay, "backend-a", 7));
    assert(restarted.execute(replay) == MediaShrinkAgentOutcome::unknown);
    assert(process.transcodes == 1);
    assert(restarted.takeReviewId().empty());

    // An FFmpeg failure is outcome-unknown once the durable stage start
    // fence exists. A restart cannot repeat the same failed stage attempt.
    auto failed = stage;
    failed.operationId = "op_ffmpeg-failed";
    failed.commandId = "cmd_ffmpeg-failed";
    failed.requestFingerprint = "request-ffmpeg-failed";
    process.failTranscode = true;
    assert(executor.authorize(failed, "backend-a", 7));
    assert(executor.execute(failed) == MediaShrinkAgentOutcome::unknown);
    process.failTranscode = false;
    const unsigned failedCount = process.transcodes;
    assert(restarted.authorize(failed, "backend-a", 7));
    assert(restarted.execute(failed) == MediaShrinkAgentOutcome::unknown);
    assert(process.transcodes == failedCount);
    assert(fs::exists(recording / "00001.ts"));

    auto publish = stage;
    publish.operationId = "op_publish-a";
    publish.commandId = "cmd_publish-a";
    publish.commandType = kMediaShrinkPublishCommandType;
    publish.payload = "{\"reviewId\":\"" + id + "\",\"revision\":1}";
    publish.verificationPolicy = "reconciliation_required";
    assert(executor.authorize(publish, "backend-a", 7));
    assert(executor.execute(publish) == MediaShrinkAgentOutcome::submitted);
    assert(executor.takeReviewId() == id);
    assert(fs::exists(root / "Example_HEVC" / recording.filename() / "00001.ts"));
    assert(fs::exists(recording / "00001.ts"));
    assert(!store.find(id)); // Candidate was atomically published.
    assert(!store.claimOnce(id, "op_publish-other"));
    assert(original == [&] {
        std::ifstream file(recording / "00001.ts", std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(file),
                           std::istreambuf_iterator<char>());
    }());
    assert(!executor.authorize(publish, "backend-a", 7));
    assert(authorizeCalls >= 4);
    std::cout << "HEVC_AGENT_STAGE_PUBLISH=PASS "
              << "STAGE_CRASH_REPLAY_FENCED=YES RECONCILIATION_REQUIRED=YES ORIGINAL_PRESERVED=YES\n";
}
