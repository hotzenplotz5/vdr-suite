#include "MediaShrink.h"

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
            const bool isOutput = args.back().find("00001.ts") != std::string::npos;
            if (contains("stream=codec_name")) {
                if (contains("a"))
                    return {0, incompatibleAudio ? "opus\n" : "aac\n"};
                return {0, isOutput ? "hevc\nhevc\n" : "h264\nh264\n"};
            }
            if (contains("format=duration"))
                return {0, isOutput && badDuration ? "10.0\n" : "60.0\n"};
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
    std::cout << "MEDIA_SHRINK_CPP_TEST=PASS checks=" << checks << "\n";
}
