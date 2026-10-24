#include "MediaImport.h"
#include "MediaImportScheduleWindow.h"

#include <cassert>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

using namespace vdrsuite::mediatools;
namespace fs = std::filesystem;

struct Fixture {
    fs::path base;
    fs::path input;
    fs::path video;
    fs::path source;

    Fixture() {
        std::string templ = (fs::temp_directory_path() / "suite-cpp-import-XXXXXX").string();
        std::vector<char> buffer(templ.begin(), templ.end());
        buffer.push_back('\0');
        char* created = ::mkdtemp(buffer.data());
        if (!created) throw std::runtime_error("mkdtemp failed");
        base = created;
        input = base / "input";
        video = base / "video";
        fs::create_directory(input);
        fs::create_directory(video);
        source = input / "Sample.mp4";
        std::ofstream(source) << "synthetic-source-unchanged";
    }
    ~Fixture() { std::error_code ec; fs::remove_all(base, ec); }
    ImportRequest request() const { return {input, video, source}; }
};

struct FakeProcess : ProcessRunner {
    std::string codec{"h264\n"};
    std::string audioCodec{"aac\n"};
    std::string outputDuration{"60.0\n"};
    bool failRemux = false;
    bool failIndex = false;
    fs::path probedSource;
    int probes = 0;
    int remuxes = 0;
    int transcodes = 0;
    int aacAudio = 0;
    int indices = 0;
    ProcessResult run(const std::vector<std::string>& argv, bool output) override {
        assert(!argv.empty());
        if (argv[0] == "/usr/bin/ffprobe") {
            assert(output);
            ++probes;
            const fs::path target = argv.back();
            const bool duration = std::find(argv.begin(), argv.end(),
                "format=duration") != argv.end();
            if (std::find(argv.begin(), argv.end(), "a") != argv.end())
                return {0, audioCodec};
            if (target.filename() == "00001.ts") {
                assert(fs::exists(target));
                if (duration) return {0, outputDuration};
                return {0, transcodes ? "h264\nh264\n" : codec + codec};
            }
            probedSource = target;
            assert(probedSource.filename() == "input.mp4");
            assert(fs::exists(probedSource));
            std::ifstream in(probedSource, std::ios::binary);
            const std::string data((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
            assert(data == "synthetic-source-unchanged");
            return {0, duration ? "60.0\n" : codec};
        }
        assert(!output);
        if (argv[0] == "/usr/bin/ffmpeg") {
            const auto input = std::find(argv.begin(), argv.end(), "-i");
            assert(input != argv.end() && (input + 1) != argv.end());
            assert(fs::path(*(input + 1)) == probedSource);
            assert(std::find(argv.begin(), argv.end(), "-fs") != argv.end());
            const bool encodeVideo =
                std::find(argv.begin(), argv.end(), "libx264") != argv.end();
            const bool encodeAudio =
                std::find(argv.begin(), argv.end(), "aac") != argv.end();
            if (encodeVideo) {
                ++transcodes;
                assert(std::find(argv.begin(), argv.end(), "yuv420p") != argv.end());
                assert(std::find(argv.begin(), argv.end(), "-g") != argv.end());
                assert(std::find(argv.begin(), argv.end(), "50") != argv.end());
            } else {
                ++remuxes;
                assert(std::find(argv.begin(), argv.end(), "copy") != argv.end());
            }
            // Track selection is independent: a VP9/AAC input needs only
            // video encoding; H.264/Opus needs only AAC conversion.
            if (encodeAudio) ++aacAudio;
            else assert(std::find(argv.begin(), argv.end(), "copy") != argv.end());
            if (failRemux) return {1, ""};
            std::ofstream(argv.back()) << "synthetic ts bytes";
            return {0, ""};
        }
        if (argv[0] == "/usr/bin/vdr") {
            ++indices;
            if (failIndex) return {1, ""};
            const std::string param = argv[1];
            assert(param.rfind("--genindex=", 0) == 0);
            const auto recording = fs::path(param.substr(11));
            assert(recording.extension() == ".rec");
            assert(fs::exists(recording / "00001.ts") &&
                   fs::exists(recording / "info"));
            std::ofstream(recording / "index") << "synthetic index";
            return {0, ""};
        }
        throw std::runtime_error("unexpected native process");
    }
};

template<class F>
void mustFail(F&& f) {
    bool threw = false;
    try { f(); } catch (const std::exception&) { threw = true; }
    assert(threw);
}

int main() {
    int checks = 0;
    {
        // Native FFmpeg/VDR children must retain the exact borrowed
        // maintenance lock FD after execv, even though it starts CLOEXEC.
        // No shell, media mutation or privileged fixture is needed.
        const int protectedFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        assert(protectedFd >= 3);
        NativeProcessRunner runner(std::chrono::seconds(3));
        const std::string fdPath =
            "/proc/self/fd/" + std::to_string(protectedFd);
        const auto probe = std::vector<std::string>{
            "/usr/bin/test", "-e", fdPath};
        assert(runner.run(probe, false).exitCode != 0);
        runner.bindMaintenanceLockFd(protectedFd);
        assert(runner.run(probe, false).exitCode == 0);
        runner.bindMaintenanceLockFd(1000000);
        assert(runner.run(probe, false).exitCode == 127);
        runner.bindMaintenanceLockFd(-1);
        assert(runner.run(probe, false).exitCode != 0);
        assert(::close(protectedFd) == 0);
        ++checks;
    }
    {
        assert(parseProbedVideoCodec("h264\nh264\n") == "h264");
        assert(parseProbedVideoCodec("hevc\r\n\r\nhevc\n") == "hevc");
        mustFail([] { parseProbedVideoCodec("h264\nhevc\n"); });
        mustFail([] { parseProbedVideoCodec(""); });
        mustFail([] { parseProbedVideoCodec("h264\n../bad\n"); });
        ++checks;
    }
    {
        using vdrsuite::mediatools::MediaImportScheduleWindow;
        const auto night = MediaImportScheduleWindow::parse("23:00-04:00");
        assert(night && night->contains(23 * 60) &&
               night->contains(3 * 60 + 59) &&
               !night->contains(4 * 60) &&
               !night->contains(22 * 60 + 59));
        const auto day = MediaImportScheduleWindow::parse("01:00-05:00");
        assert(day && day->contains(60) && day->contains(299) &&
               !day->contains(300) && !day->contains(0));
        for (const auto& malformed : {"24:00-05:00", "02:60-04:00",
                 "01:00-01:00", "1:00-05:00", "01:00/05:00",
                 "01:00-xx:00", ""}) {
            assert(!MediaImportScheduleWindow::parse(malformed));
        }
        ++checks;
    }
    {
        // An unresponsive external tool must not hang the import indefinitely.
        NativeProcessRunner runner(std::chrono::seconds(1));
        const auto result = runner.run({"/bin/sleep", "5"}, false);
        assert(result.exitCode != 0);
        ++checks;
    }
    {
        Fixture f;
        const auto plan = inspectImport(f.request());
        assert(plan.title == "Sample" && plan.sourceBytes > 0);
        assert(!fs::exists(f.video / "Sample"));
        ++checks;
    }
    {
        // Rectools workflow markers are persistent queue states: desktop
        // HandBrake delegation, explicit skip and duplicate. A source must
        // never enter the VDR import flow just because the media extension
        // after the marker is otherwise supported.
        for (const auto& name : {"Sample.pc_encode.mp4", "Sample.PC_ENCODE.mp4",
                                 "Sample.skipped.mp4", "Sample.duplicate.mp4",
                                 "Sample.pc_encode.mkv",
                                 "Die Herrschaft des Feuers.pc_encode.mkv"}) {
            Fixture f;
            const fs::path deferred = f.input / name;
            fs::rename(f.source, deferred);
            const auto before = fs::file_size(deferred);
            auto request = f.request();
            request.sourceFile = deferred;
            mustFail([&] { inspectImport(request); });
            assert(fs::exists(deferred) && fs::file_size(deferred) == before);
            assert(!fs::exists(f.video / "Sample"));
        }
        Fixture f;
        const auto normal = f.input / "Sample.pc_encoder.mp4";
        fs::rename(f.source, normal);
        auto request = f.request();
        request.sourceFile = normal;
        assert(inspectImport(request).sourceFile == normal);
        ++checks;
    }
    {
        Fixture f;
        auto r = f.request();
        r.sourceFile = f.video / "intruder.mp4";
        std::ofstream(r.sourceFile) << "x";
        mustFail([&] { inspectImport(r); });
        r = f.request();
        r.sourceFile = f.input / ".." / "input" / "Sample.mp4";
        mustFail([&] { inspectImport(r); });
        ++checks;
    }
    {
        Fixture f;
        fs::create_symlink(f.source, f.input / "Alias.mp4");
        auto r = f.request();
        r.sourceFile = f.input / "Alias.mp4";
        mustFail([&] { inspectImport(r); });
        ++checks;
    }
    {
        Fixture f;
        fs::create_directories(f.video / "Sample" / "2026-01-01.00.00.1-0.rec");
        mustFail([&] { inspectImport(f.request()); });
        ++checks;
    }
    {
        Fixture f;
        auto request = f.request();
        fs::rename(f.source, f.input / "Sample.webm");
        request.sourceFile = f.input / "Sample.webm";
        const auto accepted = inspectImport(request);
        assert(accepted.sourceFile == request.sourceFile &&
               accepted.title == "Sample");
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        auto plan = inspectImport(f.request());
        const auto output = executeRemuxImport(plan, runner, [](const ImportPlan&) {});
        assert(fs::exists(output / "00001.ts") && fs::exists(output / "index"));
        assert(fs::exists(f.source));
        assert(!fs::exists(runner.probedSource));
        assert(runner.probes == 5 && runner.remuxes == 1 && runner.indices == 1);
        assert(fs::path(output).extension() == ".rec");
        mustFail([&] { inspectImport(f.request()); });
        ++checks;
    }
    {
        Fixture f;
        std::ofstream(f.input / "Sample.nfo") <<
            "<movie><title>Example &amp; Friends</title>"
            "<outline>Short text</outline><plot>A longer description.</plot>"
            "<year>2024</year><genre>Drama</genre></movie>";
        std::ofstream(f.input / "Sample.srt") <<
            "1\n00:00:01,000 --> 00:00:02,000\nHello\n";
        std::ofstream(f.input / "Sample.de.srt") <<
            "1\n00:00:01,000 --> 00:00:02,000\nHallo\n";
        FakeProcess runner;
        const auto plan = inspectImport(f.request());
        assert(plan.title == "Example & Friends");
        const auto output = executeRemuxImport(
            plan, runner, [](const ImportPlan&) {});
        std::ifstream info(output / "info");
        const std::string contents((std::istreambuf_iterator<char>(info)),
                                    std::istreambuf_iterator<char>());
        assert(contents.find("T Example & Friends\n") != std::string::npos);
        assert(contents.find("S Short text\n") != std::string::npos);
        assert(contents.find("Year: 2024") != std::string::npos);
        assert(contents.find("Genre: Drama") != std::string::npos);
        assert(fs::exists(output / "00001.srt"));
        assert(fs::exists(output / "subtitles.de.srt"));
        assert(fs::exists(f.input / "Sample.nfo") &&
               fs::exists(f.input / "Sample.srt") &&
               fs::exists(f.input / "Sample.de.srt") &&
               fs::exists(f.source));
        ++checks;
    }
    {
        Fixture f;
        fs::create_symlink(f.source, f.input / "Sample.nfo");
        mustFail([&] { inspectImport(f.request()); });
        assert(fs::exists(f.source));
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        runner.audioCodec = "opus\n";
        const auto output = executeRemuxImport(
            inspectImport(f.request()), runner, [](const ImportPlan&) {});
        assert(fs::exists(output / "index") &&
               runner.remuxes == 1 && runner.transcodes == 0 &&
               runner.aacAudio == 1 && fs::exists(f.source));
        ++checks;
    }
    {
        Fixture f;
        for (const auto& lang : {"de", "en", "fr"}) {
            const auto subtitle = f.input / (std::string("Sample.") + lang + ".srt");
            std::ofstream(subtitle).close();
            fs::resize_file(subtitle, 8 * 1024 * 1024);
        }
        FakeProcess runner;
        const auto plan = inspectImport(f.request());
        mustFail([&] { executeRemuxImport(plan, runner,
            [](const ImportPlan&) {}); });
        assert(fs::exists(f.source) && runner.remuxes == 0 &&
               !fs::exists(f.video / "Sample"));
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        runner.codec = "vp9\n";
        auto plan = inspectImport(f.request());
        const auto output = executeRemuxImport(plan, runner, [](const ImportPlan&) {});
        assert(fs::exists(output / "00001.ts") && fs::exists(output / "index"));
        assert(fs::exists(f.source) && runner.remuxes == 0 &&
               runner.transcodes == 1);
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        runner.outputDuration = "25.0\n";
        const auto plan = inspectImport(f.request());
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {}); });
        assert(runner.indices == 0 && fs::exists(f.source) &&
               !fs::exists(f.video / "Sample"));
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        runner.failIndex = true;
        auto plan = inspectImport(f.request());
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {}); });
        assert(fs::exists(f.source) && !fs::exists(f.video / "Sample"));
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        auto plan = inspectImport(f.request());
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {
            throw std::runtime_error("capacity refused"); }); });
        assert(fs::exists(f.source) && runner.probes == 0);
        ++checks;
    }

    {
        Fixture f;
        FakeProcess runner;
        runner.failRemux = true;
        const auto plan = inspectImport(f.request());
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {}); });
        assert(fs::exists(f.source) && !fs::exists(f.video / "Sample"));
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        auto plan = inspectImport(f.request());
        std::ofstream(f.source, std::ios::app) << "source changed";
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {}); });
        assert(fs::exists(f.source) && runner.probes == 0);
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        const auto plan = inspectImport(f.request());
        const auto original = f.input / "Sample-original.mp4";
        fs::rename(f.source, original);
        // Same extension, pathname and length, but different inode.
        std::ofstream(f.source) << "synthetic-source-unchanged";
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {}); });
        assert(runner.probes == 0 && runner.remuxes == 0);
        assert(fs::exists(original) && fs::exists(f.source));
        ++checks;
    }
    std::cout << "MEDIA_IMPORT_CPP_TEST=PASS checks=" << checks << "\n";
    return 0;
}
