#include "MediaShrink.h"
#include "MediaShrinkOperationSelection.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace vdrsuite::mediatools;

struct Fixture {
    fs::path base, video, recording;
    Fixture() {
        std::string pattern =
            (fs::temp_directory_path() / "suite-shrink-test-XXXXXX").string();
        std::vector<char> name(pattern.begin(), pattern.end());
        name.push_back('\0');
        char* created = ::mkdtemp(name.data());
        if (!created) throw std::runtime_error("mkdtemp failure");
        base = created;
        video = base / "video";
        recording = video / "Sample" / "2026-10-22.10.00.1-0.rec";
        fs::create_directories(recording);
        std::ofstream(recording / "00001.ts", std::ios::binary) <<
            std::string(32768, 'X');
        std::ofstream(recording / "index") << "native-index";
        std::ofstream(recording / "info") << "T Sample\nD Description\n";
        std::ofstream(recording / "de.srt") <<
            "1\n00:00:01,000 --> 00:00:02,000\nHallo\n";
    }
    ~Fixture() {
        std::error_code error;
        fs::remove_all(base, error);
    }
    fs::path relative() const {
        return fs::path("Sample") / recording.filename();
    }
    std::string input() const {
        std::ifstream in(recording / "00001.ts", std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in),
                           std::istreambuf_iterator<char>());
    }
};

struct FakeProcess final : ProcessRunner {
    bool rejectHardware = false, failEncode = false, failIndex = false;
    bool badDuration = false, badHeight = false;
    bool discontinuousSourceTimestamps = false;
    bool divergingLiveSourceDuration = false;
    bool incompatibleAudio = false;
    int hardwareCalls = 0, softwareCalls = 0;
    int indexes = 0;
    ProcessResult run(const std::vector<std::string>& args, bool captured) override {
        assert(!args.empty());
        const auto contains = [&](const std::string& what) {
            return std::find(args.begin(), args.end(), what) != args.end();
        };
        if (args.front() == "/usr/bin/ffprobe") {
            assert(captured);
            const bool isOutput =
                args.back().find(".vdr-suite-shrink-") != std::string::npos &&
                args.back().find("00001.ts") != std::string::npos;
            if (contains("stream=codec_name")) {
                if (contains("a"))
                    return {0, incompatibleAudio ? "opus\n" : "aac\n"};
                return {0, isOutput ? "hevc\nhevc\n" : "h264\nh264\n"};
            }
            if (contains("format=duration")) {
                // The read-only concat: URL must represent the same TS
                // byte stream probed in the private Stage snapshot.
                if (args.back().rfind("concat:", 0) == 0) {
                    assert(args.back().find("00001.ts|") != std::string::npos);
                    assert(args.back().find("00002.ts") != std::string::npos);
                    return {0, divergingLiveSourceDuration ? "17.0\n" : "60.0\n"};
                }
                if (isOutput && badDuration) return {0, "10.0\n"};
                const fs::path inspected(args.back());
                // Multiple VDR segments each describe part of the 60-second
                // concatenated source, not a complete 60-second recording.
                if (inspected.filename() == "00001.ts" &&
                    !isOutput &&
                    fs::exists(inspected.parent_path() / "00002.ts"))
                    return {0, discontinuousSourceTimestamps ? "45.0\n" : "30.0\n"};
                if (inspected.filename() == "00002.ts")
                    return {0, discontinuousSourceTimestamps ? "45.0\n" : "30.0\n"};
                return {0, "60.0\n"};
            }
            if (contains("stream=height"))
                return {0, badHeight ? "1080\n720\n" : "1080\n1080\n"};
            throw std::runtime_error("unexpected FFprobe request");
        }
        assert(!captured);
        if (args.front() == "/usr/bin/ffmpeg") {
            assert(contains("0:v:0"));
            assert(contains("-fs"));
            assert(contains("-g") && contains("50"));
            const bool hw = contains("hevc_vaapi") || contains("hevc_qsv") ||
                contains("hevc_nvenc");
            if (hw) ++hardwareCalls;
            else {
                assert(contains("libx265"));
                ++softwareCalls;
            }
            if (hw && rejectHardware) return {1, ""};
            if (incompatibleAudio)
                assert(contains("aac") && contains("192k"));
            if (failEncode) return {1, ""};
            std::ofstream(args.back(), std::ios::binary) <<
                std::string(8192, 'Y');
            return {0, ""};
        }
        if (args.front() == "/usr/bin/vdr") {
            ++indexes;
            if (failIndex) return {1, ""};
            assert(args.size() == 2 && args[1].rfind("--genindex=", 0) == 0);
            const fs::path folder(args[1].substr(11));
            assert(folder.extension() == ".rec");
            assert(fs::exists(folder / "00001.ts") &&
                   fs::exists(folder / "info"));
            std::ofstream(folder / "index") << "hevc-index";
            return {0, ""};
        }
        throw std::runtime_error("unrecognized external command");
    }
};

template<typename F>
void reject(F callback) {
    bool failed = false;
    try { callback(); } catch (const std::exception&) { failed = true; }
    assert(failed);
}

int main() {
    int checks = 0;
    {
        OpaqueShrinkSelection req;
        req.recordingId = "rec_0123456789abcdef0123456789abcdef";
        assert(inspectOpaqueShrinkSelection(req) ==
               ShrinkSelectionStatus::candidateOnly);
        req.action = ShrinkRequestAction::stage;
        req.encoder = "vaapi";
        req.maxHeight = 720;
        req.nightMode = true;
        assert(inspectOpaqueShrinkSelection(req) ==
               ShrinkSelectionStatus::candidateOnly);
        req.recordingId = "../../etc/passwd";
        assert(inspectOpaqueShrinkSelection(req) ==
               ShrinkSelectionStatus::invalidRecordingId);
        req.recordingId = "rec_0123456789abcdef0123456789abcdef";
        req.maxHeight = 200;
        assert(inspectOpaqueShrinkSelection(req) ==
               ShrinkSelectionStatus::invalidOptions);
        req.maxHeight = 720;
        req.action = ShrinkRequestAction::publish;
        assert(inspectOpaqueShrinkSelection(req) ==
               ShrinkSelectionStatus::invalidCandidateId);
        req.candidateId = "shk_abcdef0123456789abcdef0123456789";
        assert(inspectOpaqueShrinkSelection(req) ==
               ShrinkSelectionStatus::invalidOptions);
        req.encoder = "software";
        req.maxHeight = 0;
        req.nightMode = false;
        assert(inspectOpaqueShrinkSelection(req) ==
               ShrinkSelectionStatus::candidateOnly);
        req.action = static_cast<ShrinkRequestAction>(99);
        assert(inspectOpaqueShrinkSelection(req) ==
               ShrinkSelectionStatus::invalidOptions);
        req.action = ShrinkRequestAction::publish;
        req.candidateId = "/tmp/evil.rec";
        assert(inspectOpaqueShrinkSelection(req) ==
               ShrinkSelectionStatus::invalidCandidateId);
        ++checks;
    }
    {
        Fixture fixture;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        assert(plan.totalBytes == 32768 && plan.segments.size() == 1);
        assert(plan.segments.front().filename == "00001.ts");
        reject([&] { inspectShrink(fixture.video, "../escape.rec"); });
        reject([&] { inspectShrink(fixture.video, "/absolute.rec"); });
        assert(fixture.input().size() == 32768);
        ++checks;
    }
    {
        Fixture fixture;
        const auto original = fixture.input();
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        FakeProcess runner;
        const auto stage = stageShrink(plan, runner, [](const ShrinkPlan&) {});
        assert(fs::exists(stage / "00001.ts") &&
               fs::exists(stage / "index") &&
               fs::exists(stage / "info") &&
               fs::exists(stage / "de.srt") &&
               fs::exists(stage / ".shrink-review") &&
               !fs::exists(stage / "input.ts"));
        assert(fixture.input() == original && runner.softwareCalls == 1);
        const auto published = publishShrinkCandidate(plan, stage, runner);
        assert(fs::exists(published / "00001.ts") &&
               fs::exists(published / "index") &&
               fs::exists(published / "de.srt") &&
               fixture.input() == original &&
               fs::exists(fixture.recording / "info") &&
               fs::exists(fixture.recording / "de.srt"));
        assert(published.string().find("_HEVC") != std::string::npos);
        reject([&] { publishShrinkCandidate(plan, stage, runner); });
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto first = inspectShrink(fixture.video, fixture.relative());
        const auto firstStage = stageShrink(first, runner,
            [](const ShrinkPlan&) {});
        const auto firstPublished = publishShrinkCandidate(
            first, firstStage, runner);
        const auto sibling = fixture.video / "Sample" /
            "2026-10-22.11.00.1-0.rec";
        fs::create_directory(sibling);
        std::ofstream(sibling / "00001.ts", std::ios::binary)
            << std::string(32768, 'X');
        std::ofstream(sibling / "info") << "T Sibling\n";
        std::ofstream(sibling / "index") << "native-index";
        const auto second = inspectShrink(fixture.video,
            fs::path("Sample") / sibling.filename());
        const auto secondStage = stageShrink(second, runner,
            [](const ShrinkPlan&) {});
        const auto secondPublished = publishShrinkCandidate(
            second, secondStage, runner);
        assert(firstPublished.parent_path() == secondPublished.parent_path());
        assert(fs::exists(firstPublished / "00001.ts"));
        assert(fs::exists(secondPublished / "00001.ts"));
        assert(fs::exists(fixture.recording / "00001.ts"));
        assert(fs::exists(sibling / "00001.ts"));
        reject([&] { publishShrinkCandidate(second, secondStage, runner); });
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto stage = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        fs::create_symlink(fixture.recording.parent_path(),
                           fixture.video / "Sample_HEVC");
        reject([&] { publishShrinkCandidate(plan, stage, runner); });
        assert(fs::exists(stage / "00001.ts"));
        assert(fs::exists(fixture.recording / "00001.ts"));
        ++checks;
    }
    {
        Fixture fixture;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        FakeProcess runner;
        runner.rejectHardware = true;
        ShrinkOptions options;
        options.encoder = ShrinkEncoder::vaapi;
        options.maxHeight = 720;
        options.nightMode = true;
        const auto configured = inspectShrink(
            fixture.video, fixture.relative(), options);
        const auto staged = stageShrink(configured, runner,
            [](const ShrinkPlan&) {});
        assert(fs::exists(staged / "00001.ts"));
        assert(runner.hardwareCalls == 1 && runner.softwareCalls == 1);
        assert(fixture.input().size() == plan.totalBytes);
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        runner.incompatibleAudio = true;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        assert(fs::exists(candidate / "00001.ts") && runner.softwareCalls == 1);
        assert(fixture.input().size() == 32768);
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        runner.badHeight = true;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        reject([&] { stageShrink(plan, runner, [](const ShrinkPlan&) {}); });
        assert(fixture.input().size() == 32768);
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        runner.badDuration = true;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        reject([&] { stageShrink(plan, runner, [](const ShrinkPlan&) {}); });
        assert(fs::exists(fixture.recording / "index"));
        assert(fixture.input().size() == 32768);
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        runner.failIndex = true;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        reject([&] { stageShrink(plan, runner, [](const ShrinkPlan&) {}); });
        assert(fixture.input().size() == 32768);
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        std::ofstream(fixture.recording / "00001.ts", std::ios::app) << "changed";
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate));
        assert(!fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        std::ofstream(fixture.recording / "info", std::ios::app) <<
            "D Metadata changed after review\n";
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate / ".shrink-review"));
        assert(fs::exists(fixture.recording / "00001.ts"));
        assert(!fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        std::ofstream(fixture.recording / "de.srt", std::ios::app) <<
            "2\n00:00:03,000 --> 00:00:04,000\nWelt\n";
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate / ".shrink-review"));
        assert(!fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        std::ofstream(candidate / "info", std::ios::binary | std::ios::trunc)
            << "T Tampered\\nD Changed\\n";
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate) && fs::exists(fixture.recording / "info"));
        assert(!fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        std::ofstream(candidate / "de.srt", std::ios::binary | std::ios::trunc)
            << "1\\n00:00:01,000 --> 00:00:02,000\\nHullo\\n";
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate) && fs::exists(fixture.recording / "de.srt"));
        assert(!fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        // A shortened candidate is rejected even with matching codec,
        // valid index and savings; the original stays untouched.
        runner.badDuration = true;
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate) && fs::exists(fixture.recording / "00001.ts"));
        assert(!fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        // A candidate must still save space at the separate publication gate.
        std::ofstream(candidate / "00001.ts", std::ios::binary | std::ios::app)
            << std::string(32768, 'Z');
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate) && fs::exists(fixture.recording / "00001.ts"));
        assert(!fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        fs::remove(fixture.recording / "00001.ts");
        fs::create_symlink(fixture.recording / "info",
                           fixture.recording / "00001.ts");
        reject([&] { inspectShrink(fixture.video, fixture.relative()); });
        ++checks;
    }
    {
        Fixture fixture;
        // A valid multi-segment source still publishes as one *additional*
        // HEVC recording after independent duration and metadata readback.
        const std::string original = fixture.input();
        std::ofstream(fixture.recording / "00002.ts", std::ios::binary)
            << std::string(32768, 'X');
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        assert(plan.segments.size() == 2);
        FakeProcess runner;
        const auto staged = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        assert(verifyShrinkCandidate(plan, staged, runner) > 0);
        const auto published = publishShrinkCandidate(plan, staged, runner);
        assert(fs::exists(published / "00001.ts") &&
               fs::exists(fixture.recording / "00001.ts") &&
               fs::exists(fixture.recording / "00002.ts") &&
               fixture.input() == original);
        ++checks;
    }
    {
        Fixture fixture;
        // Two source TS segments are combined for HEVC staging. Publication
        // must still reject a subsequently shortened candidate, not skip
        // duration verification merely because the source had segments > 1.
        std::ofstream(fixture.recording / "00002.ts", std::ios::binary)
            << std::string(32768, 'X');
        const auto source = fixture.input();
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        assert(plan.segments.size() == 2);
        FakeProcess runner;
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        runner.badDuration = true;
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate / "00001.ts") &&
               fs::exists(fixture.recording / "00002.ts") &&
               fixture.input() == source &&
               !fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        // An untrusted edit to the staged duration must not become a
        // valid publication proof even if all candidate files are present.
        const auto evidenceFile = candidate / ".shrink-review";
        std::ifstream saved(evidenceFile, std::ios::binary);
        const std::string original(
            std::istreambuf_iterator<char>{saved},
            std::istreambuf_iterator<char>{});
        const std::string marker = "DURATION/1\n";
        const auto at = original.find(marker);
        assert(at != std::string::npos);
        std::ofstream(evidenceFile, std::ios::binary | std::ios::trunc)
            << original.substr(0, at + marker.size()) << "not-a-duration\n";
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate / "00001.ts") &&
               fs::exists(fixture.recording / "00001.ts") &&
               !fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        const auto evidenceFile = candidate / ".shrink-review";
        std::ifstream input(evidenceFile, std::ios::binary);
        const std::string evidence(std::istreambuf_iterator<char>{input},
                                   std::istreambuf_iterator<char>{});
        const std::string marker = "DURATION/1\n";
        const auto offset = evidence.find(marker);
        assert(offset != std::string::npos);
        // A forged but syntactically valid duration cannot overwrite the
        // independent original single-segment probe at Publish.
        std::ofstream(evidenceFile, std::ios::binary | std::ios::trunc)
            << evidence.substr(0, offset + marker.size()) << "10.0\n";
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(fixture.recording / "00001.ts") &&
               !fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        std::ofstream(fixture.recording / "00002.ts", std::ios::binary)
            << std::string(32768, 'X');
        FakeProcess runner;
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        // A forged numeric duration is invalid even when a candidate is
        // shortened to MATCH that forged duration after Stage.
        const auto markerPath = candidate / ".shrink-review";
        std::ifstream input(markerPath, std::ios::binary);
        const std::string content(std::istreambuf_iterator<char>{input},
                                  std::istreambuf_iterator<char>{});
        const std::string marker = "DURATION/1\n";
        const auto offset = content.find(marker);
        assert(offset != std::string::npos);
        std::ofstream(markerPath, std::ios::binary | std::ios::trunc)
            << content.substr(0, offset + marker.size()) << "10.0\n";
        runner.badDuration = true;
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(fixture.recording / "00001.ts") &&
               fs::exists(fixture.recording / "00002.ts") &&
               !fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }

    {
        Fixture fixture;
        std::ofstream(fixture.recording / "00002.ts", std::ios::binary)
            << std::string(32768, 'X');
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        FakeProcess runner;
        runner.discontinuousSourceTimestamps = true;
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        // 45+45 != 60 seconds; addition of segment durations would deny
        // this candidate despite the complete read-only source probe.
        assert(verifyShrinkCandidate(plan, candidate, runner) > 0);
        const auto published = publishShrinkCandidate(plan, candidate, runner);
        assert(fs::exists(published / "00001.ts") &&
               fs::exists(fixture.recording / "00001.ts") &&
               fs::exists(fixture.recording / "00002.ts"));
        ++checks;
    }
    {
        Fixture fixture;
        std::ofstream(fixture.recording / "00002.ts", std::ios::binary)
            << std::string(32768, 'X');
        const auto plan = inspectShrink(fixture.video, fixture.relative());
        FakeProcess runner;
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        // The live concatenated source and staged marker now disagree.
        // Both review and publish must deny without changing originals.
        runner.divergingLiveSourceDuration = true;
        reject([&] { verifyShrinkCandidate(plan, candidate, runner); });
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate / "00001.ts") &&
               fs::exists(fixture.recording / "00001.ts") &&
               fs::exists(fixture.recording / "00002.ts") &&
               !fs::exists(fixture.video / "Sample_HEVC"));
        ++checks;
    }
    {
        Fixture fixture;
        // A delimiter in a recording name must never create extra inputs
        // in the FFmpeg concat: URL used for the read-only duration check.
        std::ofstream(fixture.recording / "00002.ts", std::ios::binary)
            << std::string(32768, 'X');
        const fs::path unsafe = fixture.video / "Sample|injected";
        fs::rename(fixture.recording.parent_path(), unsafe);
        const auto relative = fs::path("Sample|injected") /
            fixture.recording.filename();
        const auto plan = inspectShrink(fixture.video, relative);
        FakeProcess runner;
        const auto candidate = stageShrink(plan, runner,
            [](const ShrinkPlan&) {});
        reject([&] { verifyShrinkCandidate(plan, candidate, runner); });
        reject([&] { publishShrinkCandidate(plan, candidate, runner); });
        assert(fs::exists(candidate / "00001.ts") &&
               fs::exists(unsafe / fixture.recording.filename() / "00001.ts") &&
               fs::exists(unsafe / fixture.recording.filename() / "00002.ts"));
        ++checks;
    }
    std::cout << "MEDIA_SHRINK_CPP_TEST=PASS checks=" << checks << "\n";
}
